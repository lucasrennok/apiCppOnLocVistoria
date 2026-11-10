#include <gtest/gtest.h>
#include <json/json.h>

#include <string>

#include "support/TestSupport.h"

using namespace drogon;

TEST_F(ApiTestBase, ListAssets) {
  auto resp = testserver::get("/assets");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_TRUE(body.isArray());
  EXPECT_GE(body.size(), 3u);
}

TEST_F(ApiTestBase, GetAssetById) {
  auto resp = testserver::get("/assets/a1");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["id"].asString(), "a1");
}

TEST_F(ApiTestBase, GetMissingAssetReturns404) {
  auto resp = testserver::get("/assets/nao-existe");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k404NotFound);
}

TEST_F(ApiTestBase, CreateAndRemoveAsset) {
  Json::Value body;
  body["patrimonyNumber"] = "PAT-HTTP";
  body["serialNumber"] = "SN-HTTP";
  body["description"] = "Equipamento via HTTP";

  auto created = testserver::post("/assets", body);
  ASSERT_NE(created, nullptr);
  EXPECT_EQ(created->getStatusCode(), drogon::k201Created);
  const std::string id = testsupport::parseBody(created)["id"].asString();

  auto removed = testserver::del("/assets/" + id);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(removed)["ok"].asBool());
}

TEST_F(ApiTestBase, CreateAssetValidatesBody) {
  Json::Value body;
  body["description"] = "sem patrimonyNumber";
  auto resp = testserver::post("/assets", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);
}
