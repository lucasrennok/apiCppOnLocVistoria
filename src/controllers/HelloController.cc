#include <drogon/HttpController.h>

#include "common/Db.h"
#include "common/Http.h"

using namespace drogon;

class HelloController : public HttpController<HelloController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(HelloController::hello, "/", Get);
  ADD_METHOD_TO(HelloController::health, "/health", Get);
  METHOD_LIST_END

  void hello(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback) {
    Json::Value body;
    body["message"] = "Hello, World!";
    callback(common::json(body));
  }

  // Verifica a conexão com o MySQL
  Task<HttpResponsePtr> health(HttpRequestPtr) {
    bool up = true;
    try {
      co_await repo::db()->execSqlCoro("SELECT 1 AS ok");
    } catch (const std::exception &e) {
      LOG_ERROR << "Health check falhou: " << e.what();
      up = false;
    }
    if (!up) co_return common::messageResponse(k503ServiceUnavailable, "Banco de dados indisponível");
    Json::Value body;
    body["status"] = "ok";
    body["database"] = "connected";
    co_return common::json(body);
  }
};
