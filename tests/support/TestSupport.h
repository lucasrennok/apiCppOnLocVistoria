#pragma once

#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <ctime>
#include <sstream>
#include <string>

#include "auth/Jwt.h"
#include "common/Config.h"
#include "common/Http.h"
#include "support/DbState.h"
#include "support/TestServer.h"

// Verifica que a expressão lança common::ApiError com o status informado.
#define EXPECT_API_ERROR(expr, status_code)                                    \
  do {                                                                         \
    try {                                                                      \
      expr;                                                                    \
      FAIL() << "Esperava common::ApiError com status " << (int)(status_code); \
    } catch (const common::ApiError &e) {                                      \
      EXPECT_EQ(e.code, status_code);                                          \
    } catch (...) {                                                            \
      FAIL() << "Esperava common::ApiError, mas outra exceção foi lançada";    \
    }                                                                          \
  } while (0)

namespace testsupport {

inline Json::Value parseBody(const drogon::HttpResponsePtr &resp) {
  if (!resp) return Json::Value(Json::nullValue);
  Json::Value v;
  Json::CharReaderBuilder b;
  std::string errs;
  std::istringstream is(std::string(resp->getBody()));
  if (!Json::parseFromStream(b, is, &v, &errs)) return Json::Value(Json::nullValue);
  return v;
}

inline std::string tokenFor(const std::string &role, const std::string &sub = "u1") {
  Json::Value claims;
  claims["sub"] = sub;
  claims["role"] = role;
  claims["exp"] = static_cast<Json::Int64>(std::time(nullptr)) + 3600;
  return auth::sign(claims, common::jwtSecret());
}

inline std::string bearer(const std::string &role, const std::string &sub = "u1") {
  return "Bearer " + tokenFor(role, sub);
}

}  // namespace testsupport

// Fixture para testes que dependem do banco de dados.
class DbTestBase : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!dbtest::available()) GTEST_SKIP() << "Banco de dados indisponível";
  }
};

// Fixture para testes que passam pela aplicação HTTP (controladores/filtros).
class ApiTestBase : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!dbtest::available()) GTEST_SKIP() << "Banco de dados indisponível";
    if (!testserver::running()) GTEST_SKIP() << "Servidor HTTP de teste indisponível";
  }
};
