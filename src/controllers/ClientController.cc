#include <drogon/HttpController.h>

#include "common/Http.h"
#include "common/Util.h"
#include "repositories/ClientRepository.h"
#include "repositories/LoanRepository.h"

using namespace drogon;
using common::ApiError;

class ClientController : public HttpController<ClientController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ClientController::list, "/clients", Get);
  ADD_METHOD_TO(ClientController::create, "/clients", Post);
  ADD_METHOD_TO(ClientController::update, "/clients/{1}", Put);
  ADD_METHOD_TO(ClientController::remove, "/clients/{1}", Delete);
  ADD_METHOD_TO(ClientController::loans, "/clients/{1}/loans", Get);
  METHOD_LIST_END

  Task<HttpResponsePtr> list(HttpRequestPtr req) {
    API_BEGIN
    co_return common::json(co_await repo::clients::list(req->getParameter("search")));
    API_END
  }

  Task<HttpResponsePtr> create(HttpRequestPtr req) {
    API_BEGIN
    Json::Value out = co_await repo::clients::create(common::bodyObject(req));
    co_return common::json(out, k201Created);
    API_END
  }

  Task<HttpResponsePtr> update(HttpRequestPtr req, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::clients::update(id, common::bodyObject(req));
    if (out.isNull()) throw ApiError{k404NotFound, "Cliente não encontrado"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> remove(HttpRequestPtr, std::string id) {
    API_BEGIN
    if (!co_await repo::clients::remove(id)) throw ApiError{k404NotFound, "Cliente não encontrado"};
    co_return common::okTrue();
    API_END
  }

  Task<HttpResponsePtr> loans(HttpRequestPtr, std::string id) {
    API_BEGIN
    co_return common::json(co_await repo::loans::list("", id));
    API_END
  }
};
