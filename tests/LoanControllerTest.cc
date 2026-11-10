#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

#include "common/Db.h"
#include "support/TestSupport.h"

using namespace drogon;

namespace {
void deleteLoan(const std::string &id) {
  if (!id.empty()) repo::db()->execSqlSync("DELETE FROM loans WHERE id = ?", id);
}
}  // namespace

TEST_F(ApiTestBase, ListLoans) {
  auto resp = testserver::get("/loans");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_TRUE(body.isArray());
  EXPECT_GE(body.size(), 2u);
}

TEST_F(ApiTestBase, GetLoanById) {
  auto resp = testserver::get("/loans/l1");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_EQ(body["id"].asString(), "l1");
  EXPECT_TRUE(body["items"].isArray());
}

TEST_F(ApiTestBase, GetMissingLoanReturns404) {
  auto resp = testserver::get("/loans/nao-existe");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k404NotFound);
}

TEST_F(ApiTestBase, CreateLoanWithItems) {
  Json::Value body;
  body["clientId"] = "c1";
  body["organization"] = "Obra via HTTP";
  Json::Value items(Json::arrayValue);
  Json::Value it;
  it["assetId"] = "a3";
  it["returned"] = false;
  items.append(it);
  body["items"] = items;

  auto resp = testserver::post("/loans", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k201Created);
  const std::string id = testsupport::parseBody(resp)["id"].asString();
  EXPECT_FALSE(id.empty());

  deleteLoan(id);
}

TEST_F(ApiTestBase, CreateLoanValidatesClient) {
  Json::Value body;
  body["clientId"] = "nao-existe";
  body["organization"] = "Obra";
  auto resp = testserver::post("/loans", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);
}
