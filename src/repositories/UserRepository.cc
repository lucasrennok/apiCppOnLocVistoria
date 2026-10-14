#include "repositories/UserRepository.h"

#include "common/Config.h"
#include "common/Crypto.h"
#include "common/Db.h"
#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace repo::users {

namespace {
Json::Value toJson(const orm::Row &r) {
  Json::Value o;
  o["id"] = r["id"].as<std::string>();
  o["name"] = r["name"].as<std::string>();
  o["email"] = r["email"].as<std::string>();
  o["role"] = r["role"].as<std::string>();
  return o;
}
}  // namespace

Task<Json::Value> list() {
  auto r = co_await db()->execSqlCoro("SELECT id, name, email, role FROM users ORDER BY seq");
  Json::Value arr(Json::arrayValue);
  for (const auto &row : r) arr.append(toJson(row));
  co_return arr;
}

Task<Json::Value> create(Json::Value body) {
  std::string name = common::reqString(body, "name");
  std::string email = common::reqString(body, "email");
  std::string role = common::oneOf(common::optString(body, "role", 20).value_or("Analista"),
                                   "role", {"Analista", "Gestor"});
  if (email.find('@') == std::string::npos || email.find('.', email.find('@')) == std::string::npos)
    throw ApiError{k400BadRequest, "Campo 'email' inválido"};

  // Senha é opcional (não consta no contrato); sem ela o usuário ainda não consegue logar
  std::string hash;
  if (auto pw = common::optString(body, "password", 128)) {
    if (pw->size() < 8) throw ApiError{k400BadRequest, "Senha deve ter ao menos 8 caracteres"};
    hash = crypto::hashPassword(*pw, common::pbkdf2Iterations());
  }

  auto c = db();
  auto dup = co_await c->execSqlCoro("SELECT 1 FROM users WHERE email = ? LIMIT 1", email);
  if (!dup.empty()) throw ApiError{k409Conflict, "E-mail já cadastrado"};

  std::string id = common::newId("u");
  co_await c->execSqlCoro(
      "INSERT INTO users (id, name, email, role, password_hash) VALUES (?, ?, ?, ?, NULLIF(?, ''))",
      id, name, email, role, hash);

  auto r = co_await c->execSqlCoro("SELECT id, name, email, role FROM users WHERE id = ?", id);
  co_return toJson(r[0]);
}

Task<bool> remove(std::string id) {
  auto r = co_await db()->execSqlCoro("DELETE FROM users WHERE id = ?", id);
  co_return r.affectedRows() > 0;
}

}  // namespace repo::users
