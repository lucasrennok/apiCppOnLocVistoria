#include <gtest/gtest.h>
#include <json/json.h>

#include <string>

#include "support/TestSupport.h"

using namespace drogon;

TEST_F(ApiTestBase, ListUsersRequiresGestor) {
  auto noAuth = testserver::get("/users");
  ASSERT_NE(noAuth, nullptr);
  EXPECT_EQ(noAuth->getStatusCode(), drogon::k401Unauthorized);

  auto wrongRole = testserver::get("/users", testsupport::bearer("Analista"));
  ASSERT_NE(wrongRole, nullptr);
  EXPECT_EQ(wrongRole->getStatusCode(), drogon::k403Forbidden);

  auto ok = testserver::get("/users", testsupport::bearer("Gestor"));
  ASSERT_NE(ok, nullptr);
  EXPECT_EQ(ok->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(ok).isArray());
}

TEST_F(ApiTestBase, CreateAndRemoveUserAsGestor) {
  Json::Value body;
  body["name"] = "Usuário HTTP";
  body["email"] = "usuario.http@onloc.com.br";
  body["role"] = "Analista";
  body["password"] = "senha123";

  auto created = testserver::post("/users", body, testsupport::bearer("Gestor"));
  ASSERT_NE(created, nullptr);
  EXPECT_EQ(created->getStatusCode(), drogon::k201Created);
  const std::string id = testsupport::parseBody(created)["id"].asString();

  auto removed = testserver::del("/users/" + id, testsupport::bearer("Gestor"));
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->getStatusCode(), drogon::k200OK);
}

TEST_F(ApiTestBase, CreateUserWithoutGestorReturns403) {
  Json::Value body;
  body["name"] = "Sem Permissão";
  body["email"] = "sem.permissao@onloc.com.br";
  body["role"] = "Analista";

  auto resp = testserver::post("/users", body, testsupport::bearer("Analista"));
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k403Forbidden);
}
