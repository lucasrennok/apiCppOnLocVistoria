#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <ctime>
#include <string>

#include "auth/Jwt.h"
#include "common/Config.h"
#include "common/Crypto.h"

namespace {

Json::Value payload(long ttlSeconds = 3600) {
  Json::Value p;
  p["sub"] = "u1";
  p["role"] = "Gestor";
  p["exp"] = static_cast<Json::Int64>(std::time(nullptr)) + ttlSeconds;
  return p;
}

}  // namespace

TEST(JwtTest, SignProducesThreeParts) {
  const std::string token = auth::sign(payload(), common::jwtSecret());
  int dots = 0;
  for (char c : token)
    if (c == '.') ++dots;
  EXPECT_EQ(dots, 2);
}

TEST(JwtTest, VerifyRoundTrip) {
  const std::string token = auth::sign(payload(), common::jwtSecret());
  auto claims = auth::verify(token, common::jwtSecret());
  ASSERT_TRUE(claims.has_value());
  EXPECT_EQ((*claims)["sub"].asString(), "u1");
  EXPECT_EQ((*claims)["role"].asString(), "Gestor");
}

TEST(JwtTest, RejectsExpiredToken) {
  const std::string token = auth::sign(payload(-10), common::jwtSecret());
  EXPECT_FALSE(auth::verify(token, common::jwtSecret()).has_value());
}

TEST(JwtTest, RejectsWrongSecret) {
  const std::string token = auth::sign(payload(), common::jwtSecret());
  EXPECT_FALSE(auth::verify(token, "outro-segredo-0123456789abcdef").has_value());
}

TEST(JwtTest, RejectsTamperedSignature) {
  std::string token = auth::sign(payload(), common::jwtSecret());
  token.back() = token.back() == 'a' ? 'b' : 'a';
  EXPECT_FALSE(auth::verify(token, common::jwtSecret()).has_value());
}

TEST(JwtTest, RejectsAlgNone) {
  const std::string header = crypto::base64UrlEncode("{\"alg\":\"none\",\"typ\":\"JWT\"}");
  const std::string body = crypto::base64UrlEncode("{\"sub\":\"u1\",\"exp\":9999999999}");
  const std::string signingInput = header + "." + body;
  const std::string sig =
      crypto::base64UrlEncode(crypto::hmacSha256(common::jwtSecret(), signingInput));
  // Assinatura válida, mas header com alg "none" deve ser rejeitado.
  EXPECT_FALSE(auth::verify(signingInput + "." + sig, common::jwtSecret()).has_value());
}

TEST(JwtTest, RejectsMissingExpiration) {
  Json::Value p;
  p["sub"] = "u1";
  const std::string token = auth::sign(p, common::jwtSecret());
  EXPECT_FALSE(auth::verify(token, common::jwtSecret()).has_value());
}

TEST(JwtTest, ClaimsFromRequest) {
  const std::string token = auth::sign(payload(), common::jwtSecret());

  auto req = drogon::HttpRequest::newHttpRequest();
  req->addHeader("Authorization", "Bearer " + token);
  auto claims = auth::claimsFromRequest(req);
  ASSERT_TRUE(claims.has_value());
  EXPECT_EQ((*claims)["sub"].asString(), "u1");

  auto noHeader = drogon::HttpRequest::newHttpRequest();
  EXPECT_FALSE(auth::claimsFromRequest(noHeader).has_value());

  auto basic = drogon::HttpRequest::newHttpRequest();
  basic->addHeader("Authorization", "Basic abc");
  EXPECT_FALSE(auth::claimsFromRequest(basic).has_value());

  // O prefixo é comparado sem diferenciar maiúsculas/minúsculas.
  auto lowercase = drogon::HttpRequest::newHttpRequest();
  lowercase->addHeader("Authorization", "bearer " + token);
  EXPECT_TRUE(auth::claimsFromRequest(lowercase).has_value());
}
