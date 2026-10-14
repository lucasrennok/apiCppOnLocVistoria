#include "services/UploadService.h"

#include <cstdio>
#include <ctime>

#include "common/Config.h"
#include "common/Crypto.h"
#include "common/Util.h"

namespace uploads {

namespace {

std::string uriEncode(const std::string &s, bool encodeSlash) {
  std::string out;
  char buf[4];
  for (unsigned char c : s) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += static_cast<char>(c);
    else if (c == '/' && !encodeSlash) out += '/';
    else {
      std::snprintf(buf, sizeof buf, "%%%02X", c);
      out += buf;
    }
  }
  return out;
}

std::string sanitize(const std::string &name) {
  size_t slash = name.find_last_of("/\\");
  std::string base = slash == std::string::npos ? name : name.substr(slash + 1);
  std::string out;
  for (unsigned char c : base) out += (std::isalnum(c) || c == '.' || c == '-' || c == '_') ? static_cast<char>(c) : '_';
  while (!out.empty() && out.front() == '.') out.erase(out.begin());
  if (out.size() > 100) out = out.substr(out.size() - 100);
  return out;
}

}  // namespace

Presigned presign(const std::string &filename) {
  using common::ApiError;
  std::string clean = sanitize(filename);
  if (clean.empty()) throw ApiError{drogon::k400BadRequest, "Campo 'filename' inválido"};

  std::string endpoint = common::env("S3_PUBLIC_ENDPOINT", "http://localhost:9000");
  while (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
  std::string bucket = common::env("S3_BUCKET", "onloc");
  std::string accessKey = common::env("S3_ACCESS_KEY");
  std::string secretKey = common::env("S3_SECRET_KEY");
  std::string region = common::env("S3_REGION", "us-east-1");
  if (accessKey.empty() || secretKey.empty())
    throw ApiError{drogon::k503ServiceUnavailable, "Upload não configurado"};

  auto schemeEnd = endpoint.find("://");
  if (schemeEnd == std::string::npos) throw ApiError{drogon::k503ServiceUnavailable, "Upload não configurado"};
  std::string host = endpoint.substr(schemeEnd + 3);  // host[:porta] (endpoint sem path)

  std::string key = common::newId("").substr(0, 12) + "-" + clean;

  std::time_t t = std::time(nullptr);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char d1[16], d2[32];
  std::strftime(d1, sizeof d1, "%Y%m%d", &tm);
  std::strftime(d2, sizeof d2, "%Y%m%dT%H%M%SZ", &tm);
  std::string dateStamp = d1, amzDate = d2;

  std::string scope = dateStamp + "/" + region + "/s3/aws4_request";
  std::string canonicalUri = "/" + bucket + "/" + uriEncode(key, false);
  std::string canonicalQuery =
      "X-Amz-Algorithm=AWS4-HMAC-SHA256"
      "&X-Amz-Credential=" + uriEncode(accessKey + "/" + scope, true) +
      "&X-Amz-Date=" + amzDate +
      "&X-Amz-Expires=300"
      "&X-Amz-SignedHeaders=host";

  std::string canonicalRequest = "PUT\n" + canonicalUri + "\n" + canonicalQuery + "\nhost:" + host +
                                 "\n\nhost\nUNSIGNED-PAYLOAD";
  std::string stringToSign =
      "AWS4-HMAC-SHA256\n" + amzDate + "\n" + scope + "\n" + crypto::sha256Hex(canonicalRequest);

  std::string kDate = crypto::hmacSha256("AWS4" + secretKey, dateStamp);
  std::string kRegion = crypto::hmacSha256(kDate, region);
  std::string kService = crypto::hmacSha256(kRegion, "s3");
  std::string kSigning = crypto::hmacSha256(kService, "aws4_request");
  std::string signature = crypto::hexEncode(crypto::hmacSha256(kSigning, stringToSign));

  Presigned out;
  out.uploadUrl = endpoint + canonicalUri + "?" + canonicalQuery + "&X-Amz-Signature=" + signature;
  out.fileUrl = endpoint + canonicalUri;
  return out;
}

}  // namespace uploads
