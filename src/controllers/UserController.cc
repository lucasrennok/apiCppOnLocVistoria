#include <drogon/HttpController.h>

#include "common/Http.h"
#include "common/Util.h"
#include "repositories/UserRepository.h"

using namespace drogon;
using common::ApiError;

// Todas as rotas exigem JWT de um usuário com papel Gestor (GestorFilter)
class UserController : public HttpController<UserController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UserController::list, "/users", Get, "GestorFilter");
  ADD_METHOD_TO(UserController::create, "/users", Post, "GestorFilter");
  ADD_METHOD_TO(UserController::remove, "/users/{1}", Delete, "GestorFilter");
  METHOD_LIST_END

  Task<HttpResponsePtr> list(HttpRequestPtr) {
    API_BEGIN
    co_return common::json(co_await repo::users::list());
    API_END
  }

  Task<HttpResponsePtr> create(HttpRequestPtr req) {
    API_BEGIN
    Json::Value out = co_await repo::users::create(common::bodyObject(req));
    co_return common::json(out, k201Created);
    API_END
  }

  Task<HttpResponsePtr> remove(HttpRequestPtr, std::string id) {
    API_BEGIN
    if (!co_await repo::users::remove(id)) throw ApiError{k404NotFound, "Usuário não encontrado"};
    co_return common::okTrue();
    API_END
  }
};
