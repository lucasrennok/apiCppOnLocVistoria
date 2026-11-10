#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <stdexcept>

#include "common/Http.h"
#include "support/TestSupport.h"

TEST(HttpHelpersTest, JsonResponse) {
  Json::Value v;
  v["x"] = 1;
  auto resp = common::json(v);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["x"].asInt(), 1);
}

TEST(HttpHelpersTest, MessageResponse) {
  auto resp = common::messageResponse(drogon::k404NotFound, "não existe");
  EXPECT_EQ(resp->getStatusCode(), drogon::k404NotFound);
  EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(), "não existe");
}

TEST(HttpHelpersTest, OkTrue) {
  auto resp = common::okTrue();
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(resp)["ok"].asBool());
}

TEST(HttpHelpersTest, ErrorResponseForApiError) {
  try {
    throw common::ApiError{drogon::k404NotFound, "não existe"};
  } catch (...) {
    auto resp = common::errorResponse(std::current_exception());
    EXPECT_EQ(resp->getStatusCode(), drogon::k404NotFound);
    EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(), "não existe");
  }
}

TEST(HttpHelpersTest, ErrorResponseForGenericError) {
  try {
    throw std::runtime_error("boom");
  } catch (...) {
    auto resp = common::errorResponse(std::current_exception());
    EXPECT_EQ(resp->getStatusCode(), drogon::k500InternalServerError);
    EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(),
              "Erro interno do servidor");
  }
}

TEST(HttpHelpersTest, ErrorResponseForNull) {
  auto resp = common::errorResponse(nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k500InternalServerError);
}

namespace {

drogon::Task<drogon::HttpResponsePtr> failingHandler() {
  API_BEGIN
  throw common::ApiError{drogon::k409Conflict, "conflito"};
  co_return common::okTrue();
  API_END
}

}  // namespace

TEST(HttpHelpersTest, ApiMacrosConvertExceptionToResponse) {
  auto resp = drogon::sync_wait(failingHandler());
  EXPECT_EQ(resp->getStatusCode(), drogon::k409Conflict);
  EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(), "conflito");
}
