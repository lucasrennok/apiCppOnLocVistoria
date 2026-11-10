#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Http.h"
#include "repositories/ClientRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

class ClientRepositoryTest : public DbTestBase {};

TEST_F(ClientRepositoryTest, ListReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::clients::list(""));
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 2u);
}

TEST_F(ClientRepositoryTest, SearchMatchesTradeNameOrDocument) {
  Json::Value arr = drogon::sync_wait(repo::clients::list("alfa"));
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 1u);
  bool found = false;
  for (const auto &c : arr)
    if (c["tradeName"].asString().find("Alfa") != std::string::npos) found = true;
  EXPECT_TRUE(found);
}

TEST_F(ClientRepositoryTest, FindExistingAndMissing) {
  Json::Value c = drogon::sync_wait(repo::clients::find(repo::db(), "c1"));
  ASSERT_TRUE(c.isObject());
  EXPECT_EQ(c["id"].asString(), "c1");
  EXPECT_EQ(c["documentType"].asString(), "CNPJ");

  Json::Value missing = drogon::sync_wait(repo::clients::find(repo::db(), "nao-existe"));
  EXPECT_TRUE(missing.isNull());
}

TEST_F(ClientRepositoryTest, CreateUpdateRemove) {
  Json::Value body;
  body["responsible"] = "Responsável Teste";
  body["document"] = "11122233344";
  body["documentType"] = "CPF";
  body["tradeName"] = "Empresa de Teste LTDA";
  body["location"] = "Campinas - SP";

  Json::Value created = drogon::sync_wait(repo::clients::create(body));
  ASSERT_TRUE(created.isObject());
  const std::string id = created["id"].asString();
  EXPECT_EQ(created["document"].asString(), "11122233344");

  Json::Value upd;
  upd["tradeName"] = "Empresa Renomeada";
  Json::Value updated = drogon::sync_wait(repo::clients::update(id, upd));
  EXPECT_EQ(updated["tradeName"].asString(), "Empresa Renomeada");

  EXPECT_TRUE(drogon::sync_wait(repo::clients::remove(id)));
  EXPECT_TRUE(drogon::sync_wait(repo::clients::find(repo::db(), id)).isNull());
}

TEST_F(ClientRepositoryTest, CreateRejectsInvalidDocument) {
  Json::Value body;
  body["responsible"] = "Responsável";
  body["document"] = "abc";
  body["documentType"] = "CPF";
  body["tradeName"] = "Empresa";
  body["location"] = "Campinas";
  EXPECT_THROW(drogon::sync_wait(repo::clients::create(body)), common::ApiError);
}

TEST_F(ClientRepositoryTest, RemoveMissingReturnsFalse) {
  EXPECT_FALSE(drogon::sync_wait(repo::clients::remove("nao-existe")));
}
