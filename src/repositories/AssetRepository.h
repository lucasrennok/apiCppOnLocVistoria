#pragma once
#include <drogon/drogon.h>

#include "common/Db.h"

namespace repo::assets {
drogon::Task<Json::Value> list(std::string status);
drogon::Task<Json::Value> find(DbPtr c, std::string id);
drogon::Task<Json::Value> create(Json::Value body);
drogon::Task<Json::Value> update(std::string id, Json::Value body);
drogon::Task<bool> remove(std::string id);
}  // namespace repo::assets
