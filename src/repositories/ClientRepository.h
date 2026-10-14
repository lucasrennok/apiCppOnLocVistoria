#pragma once
#include <drogon/drogon.h>

#include "common/Db.h"

namespace repo::clients {
drogon::Task<Json::Value> list(std::string search);
drogon::Task<Json::Value> find(DbPtr c, std::string id);  // null se não existir
drogon::Task<Json::Value> create(Json::Value body);
drogon::Task<Json::Value> update(std::string id, Json::Value body);  // null se não existir
drogon::Task<bool> remove(std::string id);                           // false = não encontrado
}  // namespace repo::clients
