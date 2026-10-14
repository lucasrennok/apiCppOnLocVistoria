#pragma once
#include <algorithm>
#include <cctype>
#include <ctime>
#include <initializer_list>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "common/Http.h"

namespace common {

// ---------- IDs e datas ----------

// Ex.: "c" + 16 hex  (o doc permite UUID/auto-incremento no backend real)
inline std::string newId(const std::string &prefix) {
  std::string u = drogon::utils::getUuid();
  u.erase(std::remove(u.begin(), u.end(), '-'), u.end());
  return prefix + u.substr(0, 16);
}

// "YYYY-MM-DD HH:MM:SS" em UTC (formato DATETIME do MySQL)
inline std::string nowSql() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm);
  return buf;
}

// Aceita "YYYY-MM-DDTHH:MM:SS" com sufixo opcional ".mmm" e/ou "Z" (UTC).
// Retorna "YYYY-MM-DD HH:MM:SS" ou nullopt se inválido.
inline std::optional<std::string> isoToSql(const std::string &iso) {
  if (iso.size() < 19) return std::nullopt;
  for (size_t i = 0; i < 19; ++i) {
    char c = iso[i];
    bool ok;
    if (i == 4 || i == 7) ok = (c == '-');
    else if (i == 10) ok = (c == 'T' || c == ' ');
    else if (i == 13 || i == 16) ok = (c == ':');
    else ok = std::isdigit(static_cast<unsigned char>(c)) != 0;
    if (!ok) return std::nullopt;
  }
  if (iso.size() > 19 && iso[19] != 'Z' && iso[19] != '.') return std::nullopt;
  int mo = std::stoi(iso.substr(5, 2)), d = std::stoi(iso.substr(8, 2));
  int h = std::stoi(iso.substr(11, 2)), mi = std::stoi(iso.substr(14, 2));
  int s = std::stoi(iso.substr(17, 2));
  if (mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || s > 59) return std::nullopt;
  std::string out = iso.substr(0, 19);
  out[10] = ' ';
  return out;
}

// ---------- Validação de entrada ----------

inline Json::Value bodyObject(const drogon::HttpRequestPtr &req) {
  auto j = req->getJsonObject();
  if (!j || !j->isObject())
    throw ApiError{drogon::k400BadRequest, "Corpo da requisição deve ser um objeto JSON"};
  return *j;
}

inline std::string reqString(const Json::Value &o, const char *key, size_t maxLen = 255) {
  const Json::Value &v = o[key];
  if (!v.isString() || v.asString().empty())
    throw ApiError{drogon::k400BadRequest, std::string("Campo '") + key + "' é obrigatório"};
  if (v.asString().size() > maxLen)
    throw ApiError{drogon::k400BadRequest, std::string("Campo '") + key + "' excede o tamanho máximo"};
  return v.asString();
}

inline std::optional<std::string> optString(const Json::Value &o, const char *key,
                                            size_t maxLen = 255) {
  if (!o.isMember(key) || o[key].isNull()) return std::nullopt;
  if (!o[key].isString())
    throw ApiError{drogon::k400BadRequest, std::string("Campo '") + key + "' deve ser texto"};
  if (o[key].asString().size() > maxLen)
    throw ApiError{drogon::k400BadRequest, std::string("Campo '") + key + "' excede o tamanho máximo"};
  return o[key].asString();
}

inline std::string oneOf(const std::string &v, const char *key,
                         std::initializer_list<const char *> allowed) {
  for (auto a : allowed)
    if (v == a) return v;
  std::string list;
  for (auto a : allowed) {
    if (!list.empty()) list += " | ";
    list += a;
  }
  throw ApiError{drogon::k400BadRequest,
                 std::string("Campo '") + key + "' inválido. Valores aceitos: " + list};
}

// Escapa % _ \ para uso em LIKE ? (o valor continua sendo parâmetro vinculado)
inline std::string escapeLike(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '%' || c == '_' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

// ---------- JSON <-> texto (colunas photos) ----------

inline std::string toJsonText(const Json::Value &v) {
  Json::StreamWriterBuilder b;
  b["indentation"] = "";
  b["emitUTF8"] = true;
  return Json::writeString(b, v);
}

inline Json::Value parseJsonText(const std::string &s) {
  Json::Value v;
  Json::CharReaderBuilder b;
  std::string errs;
  std::istringstream is(s);
  if (!Json::parseFromStream(b, is, &v, &errs)) return Json::Value(Json::arrayValue);
  return v;
}

// ---------- IDs informados pelo cliente ----------
// Usa o id enviado se for seguro e estiver livre; senão gera um novo.
// `table` é sempre um literal do código (nunca entrada do usuário).
inline drogon::Task<std::string> pickId(drogon::orm::DbClientPtr c, std::string table,
                                        std::string provided, std::string prefix,
                                        std::set<std::string> *used) {
  static const std::regex re("^[A-Za-z0-9_-]{1,40}$");
  if (!provided.empty() && std::regex_match(provided, re) && !used->count(provided)) {
    auto r = co_await c->execSqlCoro("SELECT 1 FROM " + table + " WHERE id = ? LIMIT 1", provided);
    if (r.empty()) {
      used->insert(provided);
      co_return provided;
    }
  }
  std::string id = newId(prefix);
  used->insert(id);
  co_return id;
}

}  // namespace common
