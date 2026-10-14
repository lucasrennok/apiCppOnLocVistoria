#pragma once
#include <drogon/drogon.h>

namespace repo {
using DbPtr = drogon::orm::DbClientPtr;
inline DbPtr db() { return drogon::app().getDbClient(); }
}  // namespace repo
