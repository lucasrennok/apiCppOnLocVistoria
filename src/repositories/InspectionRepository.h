#pragma once
#include <drogon/drogon.h>

#include "common/Db.h"

namespace repo::inspections {
drogon::Task<Json::Value> list();
drogon::Task<Json::Value> find(DbPtr c, std::string id);
// userId: id do usuário autenticado (se houver), usado como padrão em damages[].registeredBy
drogon::Task<Json::Value> create(Json::Value body, std::string userId);
// null se a vistoria não existir
drogon::Task<Json::Value> review(std::string id, bool approve);
drogon::Task<Json::Value> listDamages();
}  // namespace repo::inspections
