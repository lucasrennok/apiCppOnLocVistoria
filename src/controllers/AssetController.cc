#include <drogon/HttpController.h>

#include "common/Db.h"
#include "common/Http.h"
#include "common/Util.h"
#include "repositories/AssetRepository.h"

using namespace drogon;
using common::ApiError;

class AssetController : public HttpController<AssetController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AssetController::list, "/assets", Get);
  ADD_METHOD_TO(AssetController::getOne, "/assets/{1}", Get);
  ADD_METHOD_TO(AssetController::create, "/assets", Post);
  ADD_METHOD_TO(AssetController::update, "/assets/{1}", Put);
  ADD_METHOD_TO(AssetController::remove, "/assets/{1}", Delete);
  METHOD_LIST_END

  Task<HttpResponsePtr> list(HttpRequestPtr req) {
    API_BEGIN
    co_return common::json(co_await repo::assets::list(req->getParameter("status")));
    API_END
  }

  Task<HttpResponsePtr> getOne(HttpRequestPtr, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::assets::find(repo::db(), id);
    if (out.isNull()) throw ApiError{k404NotFound, "Equipamento não encontrado"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> create(HttpRequestPtr req) {
    API_BEGIN
    Json::Value out = co_await repo::assets::create(common::bodyObject(req));
    co_return common::json(out, k201Created);
    API_END
  }

  Task<HttpResponsePtr> update(HttpRequestPtr req, std::string id) {
    API_BEGIN
    Json::Value out = co_await repo::assets::update(id, common::bodyObject(req));
    if (out.isNull()) throw ApiError{k404NotFound, "Equipamento não encontrado"};
    co_return common::json(out);
    API_END
  }

  Task<HttpResponsePtr> remove(HttpRequestPtr, std::string id) {
    API_BEGIN
    if (!co_await repo::assets::remove(id))
      throw ApiError{k404NotFound, "Equipamento não encontrado"};
    co_return common::okTrue();
    API_END
  }
};
