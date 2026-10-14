#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <json/json.h>

#include <exception>
#include <string>

namespace common {

// Erro "de negócio": vira resposta { "message": "..." } com o status informado
struct ApiError {
  drogon::HttpStatusCode code;
  std::string message;
};

inline drogon::HttpResponsePtr json(const Json::Value &v,
                                    drogon::HttpStatusCode code = drogon::k200OK) {
  auto resp = drogon::HttpResponse::newHttpJsonResponse(v);
  resp->setStatusCode(code);
  return resp;
}

inline drogon::HttpResponsePtr messageResponse(drogon::HttpStatusCode code,
                                               const std::string &msg) {
  Json::Value body;
  body["message"] = msg;
  return json(body, code);
}

inline drogon::HttpResponsePtr okTrue() {
  Json::Value body;
  body["ok"] = true;
  return json(body);
}

// Converte qualquer exceção capturada na resposta HTTP adequada.
// Detalhes de erro de banco vão só para o log, nunca para o cliente.
inline drogon::HttpResponsePtr errorResponse(const std::exception_ptr &ep) {
  try {
    if (ep) std::rethrow_exception(ep);
  } catch (const ApiError &e) {
    return messageResponse(e.code, e.message);
  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "Erro de banco: " << e.base().what();
  } catch (const std::exception &e) {
    LOG_ERROR << "Erro inesperado: " << e.what();
  } catch (...) {
    LOG_ERROR << "Erro desconhecido";
  }
  return messageResponse(drogon::k500InternalServerError, "Erro interno do servidor");
}

}  // namespace common

// Uso nos handlers (coroutines não permitem co_await dentro de catch):
//   Task<HttpResponsePtr> handler(HttpRequestPtr req) {
//     API_BEGIN
//     ... co_return resposta;
//     API_END
//   }
#define API_BEGIN \
  std::exception_ptr api_err; \
  try {
#define API_END \
  } catch (...) { api_err = std::current_exception(); } \
  co_return common::errorResponse(api_err);
