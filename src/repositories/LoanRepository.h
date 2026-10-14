#pragma once
#include <drogon/drogon.h>

#include "common/Db.h"

namespace repo::loans {
drogon::Task<Json::Value> list(std::string status, std::string clientId);
drogon::Task<Json::Value> find(DbPtr c, std::string id);  // null se não existir
drogon::Task<Json::Value> create(Json::Value body);
}  // namespace repo::loans
