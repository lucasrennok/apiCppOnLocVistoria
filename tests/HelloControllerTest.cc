#include <gtest/gtest.h>

#include "support/TestSupport.h"

// GET / e GET /health (HelloController).
TEST_F(ApiTestBase, RootReturnsMessage) {
  auto resp = testserver::get("/");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(), "Hello, World!");
}

TEST_F(ApiTestBase, HealthReturnsOk) {
  auto resp = testserver::get("/health");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_EQ(body["status"].asString(), "ok");
  EXPECT_EQ(body["database"].asString(), "connected");
}
