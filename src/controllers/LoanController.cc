#include <drogon/HttpController.h>

#include "common/Db.h"
#include "common/Http.h"
#include "common/Util.h"
#include "repositories/LoanRepository.h"

using namespace drogon;
using common::ApiError;

class LoanController : public HttpController<LoanController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(LoanController::list, "/loans", Get);
  ADD_METHOD_TO(LoanController::getOne, "/loans/{1}", Get);
  ADD_METHOD_TO(LoanController::create, "/loans", Post);
  METHOD_LIST_END

  Task<HttpResponsePtr> list(HttpRequestPtr req) {
    API_BEGIN
    co_return common::json(
        co_await repo::loans::list(req->getParameter("status"), req->getParameter("clientId")));
    API_END
  }

  Task<HttpResponsePtr> getOne(HttpRequestPtr, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::loans::find(repo::db(), id);
    if (out.isNull()) throw ApiError{k404NotFound, "Empréstimo não encontrado"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> create(HttpRequestPtr req) {
    API_BEGIN
    Json::Value out = co_await repo::loans::create(common::bodyObject(req));
    co_return common::json(out, k201Created);
    API_END
  }
};
