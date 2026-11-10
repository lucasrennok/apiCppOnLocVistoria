#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

#include "common/Db.h"
#include "common/Http.h"
#include "repositories/InspectionRepository.h"
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
  body["organization"] = "Obra de teste";
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

Json::Value semAvariaBody(const LoanFixture &loan) {
  Json::Value body;
  body["loanId"] = loan.id;
  body["assessment"] = "sem_avaria";
  Json::Value photos(Json::arrayValue);
  photos.append("http://exemplo/foto.png");
  body["photos"] = photos;
  return body;
}

}  // namespace

TEST_F(DbTestBase, ListReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::inspections::list());
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 2u);
}

TEST_F(DbTestBase, ListDamagesReturnsSeed) {
  Json::Value arr = drogon::sync_wait(repo::inspections::listDamages());
  ASSERT_TRUE(arr.isArray());
  EXPECT_GE(arr.size(), 1u);
}

TEST_F(DbTestBase, FindExistingAndMissing) {
  Json::Value i = drogon::sync_wait(repo::inspections::find(repo::db(), "i1"));
  ASSERT_TRUE(i.isObject());
  EXPECT_EQ(i["id"].asString(), "i1");

  Json::Value missing = drogon::sync_wait(repo::inspections::find(repo::db(), "nao-existe"));
  EXPECT_TRUE(missing.isNull());
}

TEST_F(DbTestBase, CreateSemAvaria) {
  LoanFixture loan = makeLoan();
  Json::Value insp = drogon::sync_wait(repo::inspections::create(semAvariaBody(loan), "u1"));
  ASSERT_TRUE(insp.isObject());
  EXPECT_EQ(insp["approval"].asString(), "pendente");
  EXPECT_FALSE(insp["archived"].asBool());

  cleanup(insp["id"].asString(), loan);
}

TEST_F(DbTestBase, CreateMauUsoRegistersDamageAndUpdatesLoan) {
  LoanFixture loan = makeLoan();

  Json::Value body;
  body["loanId"] = loan.id;
  body["assessment"] = "avaria_por_mau_uso";

  Json::Value items(Json::arrayValue);
  Json::Value it;
  it["id"] = loan.itemId;
  it["returned"] = true;
  items.append(it);
  body["items"] = items;

  Json::Value damages(Json::arrayValue);
  Json::Value d;
  d["loanItemId"] = loan.itemId;
  d["description"] = "Risco de teste";
  Json::Value photos(Json::arrayValue);
  photos.append("http://exemplo/dano.png");
  d["photos"] = photos;
  damages.append(d);
  body["damages"] = damages;

  Json::Value insp = drogon::sync_wait(repo::inspections::create(body, "u1"));
  ASSERT_TRUE(insp.isObject());
  EXPECT_EQ(insp["approval"].asString(), "aprovado");
  EXPECT_TRUE(insp["archived"].asBool());

  Json::Value listed = drogon::sync_wait(repo::inspections::listDamages());
  bool found = false;
  for (const auto &dmg : listed)
    if (dmg["description"].asString() == "Risco de teste") found = true;
  EXPECT_TRUE(found);

  Json::Value l = drogon::sync_wait(repo::loans::find(repo::db(), loan.id));
  ASSERT_TRUE(l.isObject());
  ASSERT_EQ(l["items"].size(), 1u);
  EXPECT_TRUE(l["items"][0]["returned"].asBool());

  cleanup(insp["id"].asString(), loan);
}

TEST_F(DbTestBase, CreateRejectsUnknownLoan) {
  Json::Value body;
  body["loanId"] = "nao-existe";
  body["assessment"] = "sem_avaria";
  EXPECT_THROW(drogon::sync_wait(repo::inspections::create(body, "u1")), common::ApiError);
}

TEST_F(DbTestBase, ReviewApproveAndReject) {
  LoanFixture loan = makeLoan();

  Json::Value insp = drogon::sync_wait(repo::inspections::create(semAvariaBody(loan), "u1"));
  const std::string id = insp["id"].asString();

  Json::Value approved = drogon::sync_wait(repo::inspections::review(id, true));
  EXPECT_EQ(approved["approval"].asString(), "aprovado");
  EXPECT_TRUE(approved["archived"].asBool());

  Json::Value rejected = drogon::sync_wait(repo::inspections::review(id, false));
  EXPECT_EQ(rejected["approval"].asString(), "reprovado");

  cleanup(id, loan);
}

TEST_F(DbTestBase, ReviewMissingReturnsNull) {
  Json::Value out = drogon::sync_wait(repo::inspections::review("nao-existe", true));
  EXPECT_TRUE(out.isNull());
}
