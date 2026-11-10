#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Http.h"
#include "repositories/UserRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

class UserRepositoryTest : public DbTestBase {};

TEST_F(UserRepositoryTest, ListReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::users::list());
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 2u);
}

TEST_F(UserRepositoryTest, CreateAndRemove) {
  Json::Value body;
  body["name"] = "Usuário de Teste";
  body["email"] = "usuario.teste@onloc.com.br";
  body["role"] = "Gestor";
  body["password"] = "senha123";

  Json::Value created = drogon::sync_wait(repo::users::create(body));
  ASSERT_TRUE(created.isObject());
  const std::string id = created["id"].asString();
  EXPECT_EQ(created["email"].asString(), "usuario.teste@onloc.com.br");
  EXPECT_EQ(created["role"].asString(), "Gestor");

  EXPECT_TRUE(drogon::sync_wait(repo::users::remove(id)));
  EXPECT_FALSE(drogon::sync_wait(repo::users::remove(id)));
}

TEST_F(UserRepositoryTest, CreateRejectsInvalidEmail) {
  Json::Value body;
  body["name"] = "Sem Arroba";
  body["email"] = "email-sem-arroba";
  EXPECT_THROW(drogon::sync_wait(repo::users::create(body)), common::ApiError);
}

TEST_F(UserRepositoryTest, CreateRejectsShortPassword) {
  Json::Value body;
  body["name"] = "Senha Curta";
  body["email"] = "senha.curta@onloc.com.br";
  body["password"] = "123";
  EXPECT_THROW(drogon::sync_wait(repo::users::create(body)), common::ApiError);
}

TEST_F(UserRepositoryTest, CreateRejectsDuplicateEmail) {
  Json::Value body;
  body["name"] = "Duplicado";
  body["email"] = "analista@onloc.com.br";  // e-mail do seed
  EXPECT_THROW(drogon::sync_wait(repo::users::create(body)), common::ApiError);
}
