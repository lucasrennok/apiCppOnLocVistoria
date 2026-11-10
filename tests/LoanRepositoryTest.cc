#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Db.h"
#include "common/Http.h"
#include "repositories/LoanRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

namespace {
void deleteLoan(const std::string &id) {
  if (!id.empty()) repo::db()->execSqlSync("DELETE FROM loans WHERE id = ?", id);
}
}  // namespace

TEST_F(DbTestBase, ListReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::loans::list("", ""));
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 2u);
}

TEST_F(DbTestBase, ListFiltersByClient) {
  Json::Value arr = drogon::sync_wait(repo::loans::list("", "c1"));
  ASSERT_TRUE(arr.isArray());
  for (const auto &l : arr) EXPECT_EQ(l["clientId"].asString(), "c1");
}

TEST_F(DbTestBase, FindExistingAndMissing) {
  Json::Value l = drogon::sync_wait(repo::loans::find(repo::db(), "l1"));
  ASSERT_TRUE(l.isObject());
  EXPECT_EQ(l["id"].asString(), "l1");
  EXPECT_TRUE(l["items"].isArray());
  EXPECT_GE(l["items"].size(), 1u);

  Json::Value missing = drogon::sync_wait(repo::loans::find(repo::db(), "nao-existe"));
  EXPECT_TRUE(missing.isNull());
}

TEST_F(DbTestBase, CreateWithItems) {
  Json::Value body;
  body["clientId"] = "c1";
  body["organization"] = "Obra de teste";
  Json::Value items(Json::arrayValue);
  Json::Value it;
  it["assetId"] = "a3";
  it["returned"] = false;
  items.append(it);
  body["items"] = items;

  Json::Value created = drogon::sync_wait(repo::loans::create(body));
  ASSERT_TRUE(created.isObject());
  const std::string id = created["id"].asString();
  EXPECT_EQ(created["status"].asString(), "sem avarias");
  ASSERT_EQ(created["items"].size(), 1u);
  EXPECT_EQ(created["items"][0]["assetId"].asString(), "a3");

  deleteLoan(id);
}

TEST_F(DbTestBase, CreateRejectsUnknownClient) {
  Json::Value body;
  body["clientId"] = "nao-existe";
  body["organization"] = "Obra";
  EXPECT_THROW(drogon::sync_wait(repo::loans::create(body)), common::ApiError);
}

TEST_F(DbTestBase, CreateRejectsUnknownAsset) {
  Json::Value body;
  body["clientId"] = "c1";
  body["organization"] = "Obra";
  Json::Value items(Json::arrayValue);
  Json::Value it;
  it["assetId"] = "nao-existe";
  items.append(it);
  body["items"] = items;
  EXPECT_THROW(drogon::sync_wait(repo::loans::create(body)), common::ApiError);
}
