#include "repositories/LoanRepository.h"

#include <map>

#include "common/Sql.h"
#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace repo::loans {

namespace {

const std::string kSelect =
    "SELECT id, client_id, organization, DATE_FORMAT(loan_date, " SQL_ISO
    ") AS loan_date, details, status FROM loans";

Json::Value itemToJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["assetId"] = r["asset_id"].as<std::string>();
  o["patrimonyNumber"] = r["patrimony_number"].as<std::string>();
  o["returned"] = r["returned"].as<int>() != 0;
  return o;
}

Json::Value toJson(const orm::Row &r, const Json::Value &items) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["clientId"] = r["client_id"].as<std::string>();
  o["organization"] = r["organization"].as<std::string>();
  o["date"] = r["loan_date"].as<std::string>();
  if (!r["details"].isNull()) o["details"] = r["details"].as<std::string>();
  o["status"] = r["status"].as<std::string>();
  o["items"] = items;
  return o;
}

}  // namespace

Task<Json::Value> list(std::string status, std::string clientId) {
  auto c = db();
  auto r = co_await c->execSqlCoro(
      kSelect + " WHERE (? = '' OR status = ?) AND (? = '' OR client_id = ?) ORDER BY seq", status,
      status, clientId, clientId);
  auto ir = co_await c->execSqlCoro(
      "SELECT id, loan_id, asset_id, patrimony_number, returned FROM loan_items ORDER BY seq");
  std::map<std::string, Json::Value> byLoan;
  for (const auto &row : ir) byLoan[row["loan_id"].as<std::string>()].append(itemToJson(row));

  Json::Value arr(Json::arrayValue);
  for (const auto &row : r) {
    auto it = byLoan.find(row["id"].as<std::string>());
    arr.append(toJson(row, it == byLoan.end() ? Json::Value(Json::arrayValue) : it->second));
  }
  co_return arr;
}

Task<Json::Value> find(DbPtr c, std::string id) {
  auto r = co_await c->execSqlCoro(kSelect + " WHERE id = ?", id);
  if (r.empty()) co_return Json::Value(Json::nullValue);
  auto ir = co_await c->execSqlCoro(
      "SELECT id, loan_id, asset_id, patrimony_number, returned FROM loan_items "
      "WHERE loan_id = ? ORDER BY seq",
      id);
  Json::Value items(Json::arrayValue);
  for (const auto &row : ir) items.append(itemToJson(row));
  co_return toJson(r[0], items);
}

Task<Json::Value> create(Json::Value body) {
  std::string clientId = common::reqString(body, "clientId", 40);
  std::string organization = common::reqString(body, "organization");
  std::string details = common::optString(body, "details", 5000).value_or("");
  std::string status = common::oneOf(common::optString(body, "status", 20).value_or("sem avarias"),
                                     "status", {"sem avarias", "com avarias"});
  std::string date = common::nowSql();
  if (auto d = common::optString(body, "date", 40)) {
    auto s = common::isoToSql(*d);
    if (!s) throw ApiError{k400BadRequest, "Campo 'date' inválido (use ISO 8601 UTC)"};
    date = *s;
  }

  // valida itens antes de abrir a transação
  struct Item { std::string id, assetId; bool returned; };
  std::vector<Item> items;
  if (body.isMember("items") && !body["items"].isNull()) {
    if (!body["items"].isArray()) throw ApiError{k400BadRequest, "Campo 'items' deve ser uma lista"};
    for (const auto &it : body["items"]) {
      if (!it.isObject()) throw ApiError{k400BadRequest, "Cada item deve ser um objeto"};
      items.push_back({common::optString(it, "id", 40).value_or(""),
                       common::reqString(it, "assetId", 40),
                       it["returned"].isBool() && it["returned"].asBool()});
    }
  }

  std::string id = common::newId("l");
  auto trans = co_await db()->newTransactionCoro();
  try {
    auto cl = co_await trans->execSqlCoro("SELECT 1 FROM clients WHERE id = ?", clientId);
    if (cl.empty()) throw ApiError{k400BadRequest, "Cliente não encontrado"};

    co_await trans->execSqlCoro(
        "INSERT INTO loans (id, client_id, organization, loan_date, details, status) "
        "VALUES (?, ?, ?, ?, NULLIF(?, ''), ?)",
        id, clientId, organization, date, details, status);

    std::set<std::string> used;
    for (auto &it : items) {
      auto a = co_await trans->execSqlCoro("SELECT patrimony_number FROM assets WHERE id = ?",
                                           it.assetId);
      if (a.empty())
        throw ApiError{k400BadRequest, "Equipamento '" + it.assetId + "' não encontrado"};
      std::string pat = a[0]["patrimony_number"].as<std::string>();
      std::string itemId = co_await common::pickId(trans, "loan_items", it.id, "li", &used);
      int returned = it.returned ? 1 : 0;
      co_await trans->execSqlCoro(
          "INSERT INTO loan_items (id, loan_id, asset_id, patrimony_number, returned) "
          "VALUES (?, ?, ?, ?, ?)",
          itemId, id, it.assetId, pat, returned);
    }
    co_return co_await find(trans, id);
  } catch (...) {
    trans->rollback();
    throw;
  }
}

}  // namespace repo::loans
