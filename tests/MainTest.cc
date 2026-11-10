#include <gtest/gtest.h>

#include "support/TestSupport.h"

// Fumaça do bootstrap (main.cc): confere que a aplicação sobe e responde nas
// rotas de raiz e de saúde, que são registradas pelos controladores.
TEST_F(ApiTestBase, RootSaysHello) {
  auto resp = testserver::get("/");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["message"].asString(), "Hello, World!");
}

TEST_F(ApiTestBase, HealthChecksDatabase) {
  auto resp = testserver::get("/health");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["status"].asString(), "ok");
  EXPECT_EQ(testsupport::parseBody(resp)["database"].asString(), "connected");
}
