#include "repositories/ClientRepository.h"

#include "common/Sql.h"
#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace repo::clients {

namespace {

const std::string kSelect =
    "SELECT id, responsible, document, document_type, trade_name, location, "
    "DATE_FORMAT(created_at, " SQL_ISO ") AS created_at FROM clients";

Json::Value toJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["responsible"] = r["responsible"].as<std::string>();
  o["document"] = r["document"].as<std::string>();
  o["documentType"] = r["document_type"].as<std::string>();
  o["tradeName"] = r["trade_name"].as<std::string>();
  o["location"] = r["location"].as<std::string>();
  o["createdAt"] = r["created_at"].as<std::string>();
  return o;
}

struct Data {
  std::string responsible, document, documentType, tradeName, location;
};

Data parse(const Json::Value &b) {
  Data d;
  d.responsible = common::reqString(b, "responsible");
  d.document = common::reqString(b, "document", 14);
  d.documentType = common::oneOf(common::reqString(b, "documentType", 4), "documentType",
                                 {"CPF", "CNPJ"});
  d.tradeName = common::reqString(b, "tradeName");
  d.location = common::reqString(b, "location");
  bool digits = std::all_of(d.document.begin(), d.document.end(),
                            [](unsigned char c) { return std::isdigit(c) != 0; });
  size_t expected = d.documentType == "CPF" ? 11 : 14;
  if (!digits || d.document.size() != expected)
    throw ApiError{k400BadRequest, "Documento inválido: informe somente dígitos (CPF=11, CNPJ=14)"};
  return d;
}

}  // namespace

Task<Json::Value> list(std::string search) {
  auto c = db();
  Json::Value arr(Json::arrayValue);
  if (search.empty()) {
    auto r = co_await c->execSqlCoro(kSelect + " ORDER BY seq");
    for (const auto &row : r) arr.append(toJson(row));
  } else {
    std::string like = "%" + common::escapeLike(search) + "%";
    auto r = co_await c->execSqlCoro(
        kSelect + " WHERE trade_name LIKE ? OR responsible LIKE ? OR document LIKE ? ORDER BY seq",
        like, like, like);
    for (const auto &row : r) arr.append(toJson(row));
  }
  co_return arr;
}

Task<Json::Value> find(DbPtr c, std::string id) {
  auto r = co_await c->execSqlCoro(kSelect + " WHERE id = ?", id);
  if (r.empty()) co_return Json::Value(Json::nullValue);
  co_return toJson(r[0]);
}

Task<Json::Value> create(Json::Value body) {
  Data d = parse(body);
  auto c = db();
  std::string id = common::newId("c");
  std::string now = common::nowSql();
  co_await c->execSqlCoro(
      "INSERT INTO clients (id, responsible, document, document_type, trade_name, location, "
      "created_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
      id, d.responsible, d.document, d.documentType, d.tradeName, d.location, now);
  co_return co_await find(c, id);
}

Task<Json::Value> update(std::string id, Json::Value body) {
  auto c = db();
  Json::Value cur = co_await find(c, id);
  if (cur.isNull()) co_return Json::Value(Json::nullValue);

  // merge parcial: só sobrescreve o que veio no body
  for (const char *k : {"responsible", "document", "documentType", "tradeName", "location"})
    if (body.isMember(k) && !body[k].isNull()) cur[k] = body[k];
  Data d = parse(cur);

  co_await c->execSqlCoro(
      "UPDATE clients SET responsible = ?, document = ?, document_type = ?, trade_name = ?, "
      "location = ? WHERE id = ?",
      d.responsible, d.document, d.documentType, d.tradeName, d.location, id);
  co_return co_await find(c, id);
}

Task<bool> remove(std::string id) {
  auto c = db();
  Json::Value cur = co_await find(c, id);
  if (cur.isNull()) co_return false;

  auto n = co_await c->execSqlCoro(
      "SELECT (SELECT COUNT(*) FROM loans WHERE client_id = ?) + "
      "(SELECT COUNT(*) FROM inspections WHERE client_id = ?) AS n",
      id, id);
  if (n[0]["n"].as<int>() > 0)
    throw ApiError{k409Conflict, "Cliente possui empréstimos ou vistorias vinculados"};

  co_await c->execSqlCoro("DELETE FROM clients WHERE id = ?", id);
  co_return true;
}

}  // namespace repo::clients
