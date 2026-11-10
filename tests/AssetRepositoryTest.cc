#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Http.h"
#include "repositories/AssetRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

class AssetRepositoryTest : public DbTestBase {};

TEST_F(AssetRepositoryTest, ListReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::assets::list(""));
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 3u);  // o seed tem 3 equipamentos
}

TEST_F(AssetRepositoryTest, ListFiltersByStatus) {
  Json::Value arr = drogon::sync_wait(repo::assets::list("disponivel"));
  for (const auto &it : arr) EXPECT_EQ(it["status"].asString(), "disponivel");
}

TEST_F(AssetRepositoryTest, FindExistingAndMissing) {
  Json::Value a = drogon::sync_wait(repo::assets::find(repo::db(), "a1"));
  ASSERT_TRUE(a.isObject());
  EXPECT_EQ(a["id"].asString(), "a1");
  EXPECT_EQ(a["patrimonyNumber"].asString(), "PAT-0001");
  EXPECT_TRUE(a["subItems"].isArray());
  EXPECT_GE(a["subItems"].size(), 1u);

  Json::Value missing = drogon::sync_wait(repo::assets::find(repo::db(), "nao-existe"));
  EXPECT_TRUE(missing.isNull());
}

TEST_F(AssetRepositoryTest, CreateUpdateRemove) {
  Json::Value body;
  body["patrimonyNumber"] = "PAT-TEST";
  body["serialNumber"] = "SN-TEST";
  body["description"] = "Equipamento de teste";
  body["status"] = "disponivel";

  Json::Value subs(Json::arrayValue);
  Json::Value s;
  s["name"] = "Cabo de teste";
  s["patrimonyNumber"] = "PAT-TEST-A";
  subs.append(s);
  body["subItems"] = subs;

  Json::Value created = drogon::sync_wait(repo::assets::create(body));
  ASSERT_TRUE(created.isObject());
  const std::string id = created["id"].asString();
  EXPECT_EQ(created["patrimonyNumber"].asString(), "PAT-TEST");
  ASSERT_EQ(created["subItems"].size(), 1u);
  EXPECT_EQ(created["subItems"][0]["name"].asString(), "Cabo de teste");

  Json::Value upd;
  upd["description"] = "Descrição atualizada";
  Json::Value updated = drogon::sync_wait(repo::assets::update(id, upd));
  EXPECT_EQ(updated["description"].asString(), "Descrição atualizada");
  EXPECT_EQ(updated["patrimonyNumber"].asString(), "PAT-TEST");

  EXPECT_TRUE(drogon::sync_wait(repo::assets::remove(id)));
  Json::Value after = drogon::sync_wait(repo::assets::find(repo::db(), id));
  EXPECT_TRUE(after.isNull());
}

TEST_F(AssetRepositoryTest, CreateValidatesRequiredFields) {
  Json::Value body;
  body["description"] = "sem patrimonyNumber";
  EXPECT_THROW(drogon::sync_wait(repo::assets::create(body)), common::ApiError);
}

TEST_F(AssetRepositoryTest, UpdateMissingReturnsNull) {
  Json::Value upd;
  upd["description"] = "qualquer";
  Json::Value out = drogon::sync_wait(repo::assets::update("nao-existe", upd));
  EXPECT_TRUE(out.isNull());
}

TEST_F(AssetRepositoryTest, RemoveMissingReturnsFalse) {
  EXPECT_FALSE(drogon::sync_wait(repo::assets::remove("nao-existe")));
}
