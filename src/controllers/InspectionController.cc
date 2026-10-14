#include <drogon/HttpController.h>

#include "auth/Jwt.h"
#include "common/Http.h"
#include "common/Util.h"
#include "repositories/InspectionRepository.h"

using namespace drogon;
using common::ApiError;

class InspectionController : public HttpController<InspectionController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(InspectionController::list, "/inspections", Get);
  ADD_METHOD_TO(InspectionController::create, "/inspections", Post);
  ADD_METHOD_TO(InspectionController::approve, "/inspections/{1}/approve", Patch, "AuthFilter");
  ADD_METHOD_TO(InspectionController::reject, "/inspections/{1}/reject", Patch, "AuthFilter");
  ADD_METHOD_TO(InspectionController::damages, "/damages", Get);
  METHOD_LIST_END

  Task<HttpResponsePtr> list(HttpRequestPtr) {
    API_BEGIN
    co_return common::json(co_await repo::inspections::list());
    API_END
  }

  Task<HttpResponsePtr> create(HttpRequestPtr req) {
    API_BEGIN
    // se houver token válido, usa o usuário como padrão em damages[].registeredBy
    std::string userId;
    if (auto claims = auth::claimsFromRequest(req)) userId = (*claims)["sub"].asString();
    Json::Value out = co_await repo::inspections::create(common::bodyObject(req), userId);
    co_return common::json(out, k201Created);
    API_END
  }

  Task<HttpResponsePtr> approve(HttpRequestPtr, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::inspections::review(id, true);
    if (out.isNull()) throw ApiError{k404NotFound, "Vistoria não encontrada"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> reject(HttpRequestPtr, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::inspections::review(id, false);
    if (out.isNull()) throw ApiError{k404NotFound, "Vistoria não encontrada"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> damages(HttpRequestPtr) {
    API_BEGIN
    co_return common::json(co_await repo::inspections::listDamages());
    API_END
  }
};
