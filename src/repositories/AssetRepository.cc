#include "repositories/AssetRepository.h"

#include <map>

#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace repo::assets {

namespace {

const std::string kSelect =
    "SELECT id, patrimony_number, serial_number, description, status FROM assets";

Json::Value subToJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["name"] = r["name"].as<std::string>();
  o["patrimonyNumber"] = r["patrimony_number"].as<std::string>();
  return o;
}

Json::Value toJson(const orm::Row &r, const Json::Value &subs) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["patrimonyNumber"] = r["patrimony_number"].as<std::string>();
  o["serialNumber"] = r["serial_number"].as<std::string>();
  o["description"] = r["description"].as<std::string>();
  o["subItems"] = subs;
  o["status"] = r["status"].as<std::string>();
  return o;
}

struct Data {
  std::string patrimonyNumber, serialNumber, description, status;
  Json::Value subItems{Json::arrayValue};  // [{id?, name, patrimonyNumber}]
};

Data parse(const Json::Value &b) {
  Data d;
  d.patrimonyNumber = common::reqString(b, "patrimonyNumber", 50);
  d.serialNumber = common::optString(b, "serialNumber", 100).value_or("");
  d.description = common::reqString(b, "description", 500);
  d.status = common::oneOf(common::optString(b, "status", 20).value_or("disponivel"), "status",
                           {"disponivel", "emprestado", "avariado"});
  if (b.isMember("subItems") && !b["subItems"].isNull()) {
    if (!b["subItems"].isArray())
      throw ApiError{k400BadRequest, "Campo 'subItems' deve ser uma lista"};
    for (const auto &it : b["subItems"]) {
      if (!it.isObject()) throw ApiError{k400BadRequest, "Cada subItem deve ser um objeto"};
      Json::Value s;
      s["id"] = common::optString(it, "id", 40).value_or("");
      s["name"] = common::reqString(it, "name");
      s["patrimonyNumber"] = common::optString(it, "patrimonyNumber", 50).value_or("");
      d.subItems.append(s);
    }
  }
  return d;
}

// Substitui todos os subitens do equipamento
Task<> writeSubItems(DbPtr c, std::string assetId, Json::Value items) {
  co_await c->execSqlCoro("DELETE FROM asset_sub_items WHERE asset_id = ?", assetId);
  std::set<std::string> used;
  for (const auto &it : items) {
    std::string id = co_await common::pickId(c, "asset_sub_items", it["id"].asString(), "s", &used);
    std::string name = it["name"].asString();
    std::string pat = it["patrimonyNumber"].asString();
    co_await c->execSqlCoro(
        "INSERT INTO asset_sub_items (id, asset_id, name, patrimony_number) VALUES (?, ?, ?, ?)",
        id, assetId, name, pat);
  }
}

}  // namespace

Task<Json::Value> list(std::string status) {
  auto c = db();
  auto r = co_await c->execSqlCoro(kSelect + " WHERE (? = '' OR status = ?) ORDER BY seq", status,
                                   status);
  auto sr = co_await c->execSqlCoro(
      "SELECT id, asset_id, name, patrimony_number FROM asset_sub_items ORDER BY seq");
  std::map<std::string, Json::Value> byAsset;
  for (const auto &row : sr) byAsset[row["asset_id"].as<std::string>()].append(subToJson(row));

  Json::Value arr(Json::arrayValue);
  for (const auto &row : r) {
    auto it = byAsset.find(row["id"].as<std::string>());
    arr.append(toJson(row, it == byAsset.end() ? Json::Value(Json::arrayValue) : it->second));
  }
  co_return arr;
}

Task<Json::Value> find(DbPtr c, std::string id) {
  auto r = co_await c->execSqlCoro(kSelect + " WHERE id = ?", id);
  if (r.empty()) co_return Json::Value(Json::nullValue);
  auto sr = co_await c->execSqlCoro(
      "SELECT id, name, patrimony_number FROM asset_sub_items WHERE asset_id = ? ORDER BY seq", id);
  Json::Value subs(Json::arrayValue);
  for (const auto &row : sr) subs.append(subToJson(row));
  co_return toJson(r[0], subs);
}

Task<Json::Value> create(Json::Value body) {
  Data d = parse(body);
  std::string id = common::newId("a");
  auto trans = co_await db()->newTransactionCoro();
  try {
    co_await trans->execSqlCoro(
        "INSERT INTO assets (id, patrimony_number, serial_number, description, status) "
        "VALUES (?, ?, ?, ?, ?)",
        id, d.patrimonyNumber, d.serialNumber, d.description, d.status);
    co_await writeSubItems(trans, id, d.subItems);
    co_return co_await find(trans, id);
  } catch (...) {
    trans->rollback();
    throw;
  }
}

Task<Json::Value> update(std::string id, Json::Value body) {
  auto trans = co_await db()->newTransactionCoro();
  try {
    Json::Value cur = co_await find(trans, id);
    if (cur.isNull()) co_return Json::Value(Json::nullValue);

    bool replaceSubs = body.isMember("subItems") && !body["subItems"].isNull();
    for (const char *k : {"patrimonyNumber", "serialNumber", "description", "status"})
      if (body.isMember(k) && !body[k].isNull()) cur[k] = body[k];
    if (replaceSubs) cur["subItems"] = body["subItems"];
    Data d = parse(cur);

    co_await trans->execSqlCoro(
        "UPDATE assets SET patrimony_number = ?, serial_number = ?, description = ?, status = ? "
        "WHERE id = ?",
        d.patrimonyNumber, d.serialNumber, d.description, d.status, id);
    if (replaceSubs) co_await writeSubItems(trans, id, d.subItems);
    co_return co_await find(trans, id);
  } catch (...) {
    trans->rollback();
    throw;
  }
}

Task<bool> remove(std::string id) {
  auto c = db();
  Json::Value cur = co_await find(c, id);
  if (cur.isNull()) co_return false;
  auto n = co_await c->execSqlCoro("SELECT COUNT(*) AS n FROM loan_items WHERE asset_id = ?", id);
  if (n[0]["n"].as<int>() > 0)
    throw ApiError{k409Conflict, "Equipamento possui empréstimos vinculados"};
  co_await c->execSqlCoro("DELETE FROM assets WHERE id = ?", id);
  co_return true;
}

}  // namespace repo::assets
