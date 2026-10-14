#pragma once
#include <drogon/drogon.h>

namespace repo::users {
drogon::Task<Json::Value> list();
drogon::Task<Json::Value> create(Json::Value body);
drogon::Task<bool> remove(std::string id);  // false = não encontrado
}  // namespace repo::users
