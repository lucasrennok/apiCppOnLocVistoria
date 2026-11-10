#include <gtest/gtest.h>
#include <json/json.h>

#include <string>

#include "support/TestSupport.h"

using namespace drogon;

TEST_F(ApiTestBase, ListClients) {
  auto resp = testserver::get("/clients");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_TRUE(body.isArray());
  EXPECT_GE(body.size(), 2u);
}

TEST_F(ApiTestBase, GetClientById) {
  auto resp = testserver::get("/clients/c1");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(resp)["id"].asString(), "c1");
}

TEST_F(ApiTestBase, GetClientLoans) {
  auto resp = testserver::get("/clients/c1/loans");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value body = testsupport::parseBody(resp);
  EXPECT_TRUE(body.isArray());
  EXPECT_GE(body.size(), 1u);
}

TEST_F(ApiTestBase, CreateAndRemoveClient) {
  Json::Value body;
  body["responsible"] = "Responsável HTTP";
  body["document"] = "55566677788";
  body["documentType"] = "CPF";
  body["tradeName"] = "Empresa HTTP LTDA";
  body["location"] = "Campinas - SP";

  auto created = testserver::post("/clients", body);
  ASSERT_NE(created, nullptr);
  EXPECT_EQ(created->getStatusCode(), drogon::k201Created);
  const std::string id = testsupport::parseBody(created)["id"].asString();

  auto removed = testserver::del("/clients/" + id);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(removed->getStatusCode(), drogon::k200OK);
}

TEST_F(ApiTestBase, CreateClientValidatesDocument) {
  Json::Value body;
  body["responsible"] = "Responsável";
  body["document"] = "abc";
  body["documentType"] = "CPF";
  body["tradeName"] = "Empresa";
  body["location"] = "Campinas";
  auto resp = testserver::post("/clients", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);
}
