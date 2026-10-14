#include "repositories/InspectionRepository.h"

#include "common/Sql.h"
#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace repo::inspections {

namespace {

const std::string kSelect =
    "SELECT id, client_id, loan_id, DATE_FORMAT(inspection_date, " SQL_ISO
    ") AS inspection_date, photos, assessment, approval, archived FROM inspections";

Json::Value toJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["clientId"] = r["client_id"].as<std::string>();
  o["loanId"] = r["loan_id"].as<std::string>();
  o["date"] = r["inspection_date"].as<std::string>();
  o["photos"] = common::parseJsonText(r["photos"].as<std::string>());
  o["assessment"] = r["assessment"].as<std::string>();
  o["approval"] = r["approval"].as<std::string>();
  o["archived"] = r["archived"].as<int>() != 0;
  return o;
}

// PhotoEvidence[]: [{ url, note? }]  (strings soltas também são aceitas)
Json::Value parsePhotos(const Json::Value &b) {
  Json::Value out(Json::arrayValue);
  if (!b.isMember("photos") || b["photos"].isNull()) return out;
  if (!b["photos"].isArray()) throw ApiError{k400BadRequest, "Campo 'photos' deve ser uma lista"};
  for (const auto &p : b["photos"]) {
    Json::Value e;
    if (p.isString()) {
      e["url"] = p.asString();
    } else if (p.isObject()) {
      e["url"] = common::reqString(p, "url", 2000);
      if (auto n = common::optString(p, "note", 1000)) e["note"] = *n;
    } else {
      throw ApiError{k400BadRequest, "Foto inválida em 'photos'"};
    }
    out.append(e);
  }
  return out;
}

// Damage.photos: string[]
Json::Value parseUrlList(const Json::Value &b, const char *key) {
  Json::Value out(Json::arrayValue);
  if (!b.isMember(key) || b[key].isNull()) return out;
  if (!b[key].isArray())
    throw ApiError{k400BadRequest, std::string("Campo '") + key + "' deve ser uma lista"};
  for (const auto &p : b[key]) {
    if (!p.isString()) throw ApiError{k400BadRequest, std::string("Itens de '") + key + "' devem ser texto"};
    out.append(p.asString());
  }
  return out;
}

Json::Value damageToJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["loanId"] = r["loan_id"].as<std::string>();
  o["loanItemId"] = r["loan_item_id"].as<std::string>();
  o["description"] = r["description"].as<std::string>();
  o["photos"] = common::parseJsonText(r["photos"].as<std::string>());
  o["registeredBy"] = r["registered_by"].as<std::string>();
  o["registeredAt"] = r["registered_at"].as<std::string>();
  return o;
}

}  // namespace

Task<Json::Value> list() {
  auto r = co_await db()->execSqlCoro(kSelect + " ORDER BY seq");
  Json::Value arr(Json::arrayValue);
  for (const auto &row : r) arr.append(toJson(row));
  co_return arr;
}

Task<Json::Value> find(DbPtr c, std::string id) {
  auto r = co_await c->execSqlCoro(kSelect + " WHERE id = ?", id);
  if (r.empty()) co_return Json::Value(Json::nullValue);
  co_return toJson(r[0]);
}

Task<Json::Value> create(Json::Value body, std::string userId) {
  std::string loanId = common::reqString(body, "loanId", 40);
  std::string assessment = common::oneOf(common::reqString(body, "assessment", 30), "assessment",
                                         {"avaria_por_mau_uso", "sem_avaria"});
  bool misuse = assessment == "avaria_por_mau_uso";
  Json::Value photos = parsePhotos(body);

  std::string date = common::nowSql();
  if (auto d = common::optString(body, "date", 40)) {
    auto s = common::isoToSql(*d);
    if (!s) throw ApiError{k400BadRequest, "Campo 'date' inválido (use ISO 8601 UTC)"};
    date = *s;
  }

  // items[]: { id, returned } -> atualiza a flag "returned" dos itens do empréstimo
  struct ItemUpd { std::string id; int returned; };
  std::vector<ItemUpd> itemUpdates;
  if (body.isMember("items") && !body["items"].isNull()) {
    if (!body["items"].isArray()) throw ApiError{k400BadRequest, "Campo 'items' deve ser uma lista"};
    for (const auto &it : body["items"]) {
      if (!it.isObject()) throw ApiError{k400BadRequest, "Cada item deve ser um objeto"};
      itemUpdates.push_back({common::reqString(it, "id", 40),
                             (it["returned"].isBool() && it["returned"].asBool()) ? 1 : 0});
    }
  }

  // damages[]: só são registradas quando há mau uso
  struct Dmg { std::string loanItemId, description, photosText, registeredBy; };
  std::vector<Dmg> damages;
  if (misuse && body.isMember("damages") && !body["damages"].isNull()) {
    if (!body["damages"].isArray()) throw ApiError{k400BadRequest, "Campo 'damages' deve ser uma lista"};
    for (const auto &dm : body["damages"]) {
      if (!dm.isObject()) throw ApiError{k400BadRequest, "Cada avaria deve ser um objeto"};
      std::string by = common::optString(dm, "registeredBy", 40).value_or(userId);
      if (by.empty()) throw ApiError{k400BadRequest, "Campo 'registeredBy' é obrigatório"};
      damages.push_back({common::reqString(dm, "loanItemId", 40),
                         common::reqString(dm, "description", 5000),
                         common::toJsonText(parseUrlList(dm, "photos")), by});
    }
  }

  std::string id = common::newId("i");
  std::string photosText = common::toJsonText(photos);
  std::string approval = misuse ? "aprovado" : "pendente";
  int archived = misuse ? 1 : 0;

  auto trans = co_await db()->newTransactionCoro();
  try {
    auto loan = co_await trans->execSqlCoro("SELECT client_id FROM loans WHERE id = ?", loanId);
    if (loan.empty()) throw ApiError{k404NotFound, "Empréstimo não encontrado"};
    std::string clientId = common::optString(body, "clientId", 40)
                               .value_or(loan[0]["client_id"].as<std::string>());
    auto cl = co_await trans->execSqlCoro("SELECT 1 FROM clients WHERE id = ?", clientId);
    if (cl.empty()) throw ApiError{k400BadRequest, "Cliente não encontrado"};

    for (const auto &u : itemUpdates) {
      auto ex = co_await trans->execSqlCoro(
          "SELECT 1 FROM loan_items WHERE id = ? AND loan_id = ?", u.id, loanId);
      if (ex.empty())
        throw ApiError{k400BadRequest, "Item '" + u.id + "' não pertence ao empréstimo"};
      int ret = u.returned;
      std::string itemId = u.id;
      co_await trans->execSqlCoro("UPDATE loan_items SET returned = ? WHERE id = ?", ret, itemId);
    }

    co_await trans->execSqlCoro(
        "INSERT INTO inspections (id, client_id, loan_id, inspection_date, photos, assessment, "
        "approval, archived) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        id, clientId, loanId, date, photosText, assessment, approval, archived);

    if (misuse) {
      co_await trans->execSqlCoro("UPDATE loans SET status = 'com avarias' WHERE id = ?", loanId);
      std::string now = common::nowSql();
      for (const auto &d : damages) {
        auto ex = co_await trans->execSqlCoro(
            "SELECT 1 FROM loan_items WHERE id = ? AND loan_id = ?", d.loanItemId, loanId);
        if (ex.empty())
          throw ApiError{k400BadRequest, "Item '" + d.loanItemId + "' não pertence ao empréstimo"};
        std::string dmgId = common::newId("d");
        co_await trans->execSqlCoro(
            "INSERT INTO damages (id, loan_id, loan_item_id, description, photos, registered_by, "
            "registered_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
            dmgId, loanId, d.loanItemId, d.description, d.photosText, d.registeredBy, now);
      }
    }
    co_return co_await find(trans, id);
  } catch (...) {
    trans->rollback();
    throw;
  }
}

Task<Json::Value> review(std::string id, bool approve) {
  auto trans = co_await db()->newTransactionCoro();
  try {
    Json::Value cur = co_await find(trans, id);
    if (cur.isNull()) co_return Json::Value(Json::nullValue);

    if (approve) {
      std::string loanId = cur["loanId"].asString();
      co_await trans->execSqlCoro(
          "UPDATE inspections SET approval = 'aprovado', archived = 1 WHERE id = ?", id);
      co_await trans->execSqlCoro("UPDATE loans SET status = 'sem avarias' WHERE id = ?", loanId);
    } else {
      co_await trans->execSqlCoro("UPDATE inspections SET approval = 'reprovado' WHERE id = ?", id);
    }
    co_return co_await find(trans, id);
  } catch (...) {
    trans->rollback();
    throw;
  }
}

Task<Json::Value> listDamages() {
  auto r = co_await db()->execSqlCoro(
      "SELECT id, loan_id, loan_item_id, description, photos, registered_by, "
      "DATE_FORMAT(registered_at, " SQL_ISO ") AS registered_at FROM damages ORDER BY seq");
  Json::Value arr(Json::arrayValue);
  for (const auto &row : r) arr.append(damageToJson(row));
  co_return arr;
}

}  // namespace repo::inspections
