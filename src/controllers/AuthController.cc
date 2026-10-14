#include <drogon/HttpController.h>

#include <ctime>

#include "auth/Jwt.h"
#include "common/Config.h"
#include "common/Crypto.h"
#include "common/Db.h"
#include "common/Http.h"
#include "common/Util.h"

using namespace drogon;
using common::ApiError;

namespace {
// Hash fictício: usado para gastar o mesmo tempo quando o e-mail não existe
const char *kDummyHash =
    "pbkdf2$210000$00000000000000000000000000000000$"
    "0000000000000000000000000000000000000000000000000000000000000000";
constexpr long kTokenTtlSeconds = 3600;  // 1 hora
}  // namespace

class AuthController : public HttpController<AuthController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthController::login, "/auth/login", Post);
  ADD_METHOD_TO(AuthController::logout, "/auth/logout", Post);
  METHOD_LIST_END

  Task<HttpResponsePtr> login(HttpRequestPtr req) {
    API_BEGIN
    Json::Value body = common::bodyObject(req);
    std::string email = common::reqString(body, "email");
    std::string password = common::reqString(body, "password", 256);

    auto r = co_await repo::db()->execSqlCoro(
        "SELECT id, name, email, role, password_hash FROM users WHERE email = ?", email);

    bool ok = false;
    if (r.empty() || r[0]["password_hash"].isNull()) {
      crypto::verifyPassword(password, kDummyHash);
    } else {
      ok = crypto::verifyPassword(password, r[0]["password_hash"].as<std::string>());
    }
    if (!ok) throw ApiError{k401Unauthorized, "Credenciais inválidas"};

    Json::Value user;
    user["id"] = r[0]["id"].as<std::string>();
    user["name"] = r[0]["name"].as<std::string>();
    user["email"] = r[0]["email"].as<std::string>();
    user["role"] = r[0]["role"].as<std::string>();

    Json::Int64 now = static_cast<Json::Int64>(std::time(nullptr));
    Json::Value claims;
    claims["sub"] = user["id"];
    claims["email"] = user["email"];
    claims["role"] = user["role"];
    claims["iat"] = now;
    claims["exp"] = now + kTokenTtlSeconds;

    Json::Value out;
    out["token"] = auth::sign(claims, common::jwtSecret());
    out["user"] = user;
    co_return common::json(out);
    API_END
  }

  // JWT é stateless: o cliente descarta o token
  void logout(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback) {
    callback(common::okTrue());
  }
};
