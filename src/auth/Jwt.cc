#include "auth/Jwt.h"

#include <strings.h>

#include <ctime>
#include <sstream>

#include "common/Config.h"
#include "common/Crypto.h"
#include "common/Util.h"

namespace auth {

std::string sign(const Json::Value &payload, const std::string &secret) {
  Json::Value header;
  header["alg"] = "HS256";
  header["typ"] = "JWT";
  std::string signingInput = crypto::base64UrlEncode(common::toJsonText(header)) + "." +
                             crypto::base64UrlEncode(common::toJsonText(payload));
  return signingInput + "." + crypto::base64UrlEncode(crypto::hmacSha256(secret, signingInput));
}

std::optional<Json::Value> verify(const std::string &token, const std::string &secret) {
  auto d1 = token.find('.');
  if (d1 == std::string::npos) return std::nullopt;
  auto d2 = token.find('.', d1 + 1);
  if (d2 == std::string::npos || token.find('.', d2 + 1) != std::string::npos) return std::nullopt;

  std::string signingInput = token.substr(0, d2);
  std::string sig = token.substr(d2 + 1);

  // assinatura (comparação em tempo constante)
  std::string expected = crypto::base64UrlEncode(crypto::hmacSha256(secret, signingInput));
  if (!crypto::constantTimeEquals(expected, sig)) return std::nullopt;

  std::string headerTxt, payloadTxt;
  if (!crypto::base64UrlDecode(token.substr(0, d1), headerTxt) ||
      !crypto::base64UrlDecode(token.substr(d1 + 1, d2 - d1 - 1), payloadTxt))
    return std::nullopt;

  Json::Value header = common::parseJsonText(headerTxt);
  Json::Value payload = common::parseJsonText(payloadTxt);
  if (!header.isObject() || !payload.isObject()) return std::nullopt;
  if (header["alg"].asString() != "HS256") return std::nullopt;  // rejeita "none" etc.
  if (!payload["exp"].isIntegral() ||
      payload["exp"].asInt64() <= static_cast<Json::Int64>(std::time(nullptr)))
    return std::nullopt;
  return payload;
}

std::optional<Json::Value> claimsFromRequest(const drogon::HttpRequestPtr &req) {
  const std::string h = req->getHeader("Authorization");
  static const std::string prefix = "Bearer ";
  if (h.size() <= prefix.size() || strncasecmp(h.c_str(), prefix.c_str(), prefix.size()) != 0)
    return std::nullopt;
  return verify(h.substr(prefix.size()), common::jwtSecret());
}

}  // namespace auth
