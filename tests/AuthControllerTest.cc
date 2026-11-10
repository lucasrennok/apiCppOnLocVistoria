#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "repositories/UserRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

TEST_F(ApiTestBase, LoginWithValidCredentials) {
  Json::Value user;
  user["name"] = "Login Teste";
  user["email"] = "login.teste@onloc.com.br";
  user["role"] = "Analista";
  user["password"] = "senha123";
  Json::Value created = drogon::sync_wait(repo::users::create(user));
  const std::string uid = created["id"].asString();

  Json::Value okBody;
  okBody["email"] = "login.teste@onloc.com.br";
  okBody["password"] = "senha123";
  auto resp = testserver::post("/auth/login", okBody);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_FALSE(body["token"].asString().empty());
  EXPECT_EQ(body["user"]["email"].asString(), "login.teste@onloc.com.br");

  Json::Value badBody;
  badBody["email"] = "login.teste@onloc.com.br";
  badBody["password"] = "senha-errada";
  auto bad = testserver::post("/auth/login", badBody);
  ASSERT_NE(bad, nullptr);
  EXPECT_EQ(bad->getStatusCode(), drogon::k401Unauthorized);

  drogon::sync_wait(repo::users::remove(uid));
}

TEST_F(ApiTestBase, LoginRejectsMissingFields) {
  Json::Value body;
  body["email"] = "alguem@onloc.com.br";
  auto resp = testserver::post("/auth/login", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);
}

TEST_F(ApiTestBase, LogoutReturnsOk) {
  auto resp = testserver::post("/auth/logout", Json::Value(Json::objectValue));
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(resp)["ok"].asBool());
}
