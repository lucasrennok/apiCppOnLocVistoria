#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Db.h"
#include "repositories/LoanRepository.h"
#include "support/TestSupport.h"

using namespace drogon;

namespace {

struct LoanFixture {
  std::string id;
  std::string itemId;
};

LoanFixture makeLoan() {
  Json::Value body;
  body["clientId"] = "c1";
  body["organization"] = "Obra de teste HTTP";
  Json::Value items(Json::arrayValue);
  Json::Value it;
  it["assetId"] = "a3";
  it["returned"] = false;
  items.append(it);
  body["items"] = items;

  Json::Value created = drogon::sync_wait(repo::loans::create(body));
  LoanFixture f;
  f.id = created["id"].asString();
  f.itemId = created["items"][0]["id"].asString();
  return f;
}

void cleanup(const std::string &inspectionId, const LoanFixture &loan) {
  if (!inspectionId.empty())
    repo::db()->execSqlSync("DELETE FROM inspections WHERE id = ?", inspectionId);
  if (!loan.id.empty()) repo::db()->execSqlSync("DELETE FROM loans WHERE id = ?", loan.id);
}

}  // namespace

TEST_F(ApiTestBase, ListInspections) {
  auto resp = testserver::get("/inspections");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(resp).isArray());
}

TEST_F(ApiTestBase, ListDamages) {
  auto resp = testserver::get("/damages");
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  EXPECT_TRUE(testsupport::parseBody(resp).isArray());
}

TEST_F(ApiTestBase, CreateInspection) {
  LoanFixture loan = makeLoan();
  Json::Value body;
  body["loanId"] = loan.id;
  body["assessment"] = "sem_avaria";

  auto resp = testserver::post("/inspections", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k201Created);
  const std::string id = testsupport::parseBody(resp)["id"].asString();

  cleanup(id, loan);
}

TEST_F(ApiTestBase, ApproveRequiresGestorRole) {
  LoanFixture loan = makeLoan();

  Json::Value body;
  body["loanId"] = loan.id;
  body["assessment"] = "sem_avaria";
  auto created = testserver::post("/inspections", body);
  const std::string id = testsupport::parseBody(created)["id"].asString();
  const std::string path = "/inspections/" + id + "/approve";

  // Sem token -> 401; com papel errado -> 403; com Gestor -> 200.
  auto noAuth = testserver::patch(path);
  ASSERT_NE(noAuth, nullptr);
  EXPECT_EQ(noAuth->getStatusCode(), drogon::k401Unauthorized);

  auto wrongRole = testserver::patch(path, testsupport::bearer("Analista"));
  ASSERT_NE(wrongRole, nullptr);
  EXPECT_EQ(wrongRole->getStatusCode(), drogon::k403Forbidden);

  auto ok = testserver::patch(path, testsupport::bearer("Gestor"));
  ASSERT_NE(ok, nullptr);
  EXPECT_EQ(ok->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(ok)["approval"].asString(), "aprovado");

  cleanup(id, loan);
}

TEST_F(ApiTestBase, RejectRequiresGestorRole) {
  LoanFixture loan = makeLoan();

  Json::Value body;
  body["loanId"] = loan.id;
  body["assessment"] = "sem_avaria";
  auto created = testserver::post("/inspections", body);
  const std::string id = testsupport::parseBody(created)["id"].asString();
  const std::string path = "/inspections/" + id + "/reject";

  auto ok = testserver::patch(path, testsupport::bearer("Gestor"));
  ASSERT_NE(ok, nullptr);
  EXPECT_EQ(ok->getStatusCode(), drogon::k200OK);
  EXPECT_EQ(testsupport::parseBody(ok)["approval"].asString(), "reprovado");

  cleanup(id, loan);
}
