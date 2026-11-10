#pragma once

#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>

#include <string>

// Sobe a aplicação em uma thread separada (com listener local) para os testes
// de controladores e fornece helpers HTTP síncronos.
namespace testserver {

void start(int port);
void stop();
bool running();

drogon::HttpResponsePtr request(drogon::HttpMethod method,
                                const std::string &path,
                                const std::string &body = "",
                                const std::string &authHeader = "");

drogon::HttpResponsePtr get(const std::string &path, const std::string &auth = "");
drogon::HttpResponsePtr post(const std::string &path, const Json::Value &body,
                             const std::string &auth = "");
drogon::HttpResponsePtr put(const std::string &path, const Json::Value &body,
                            const std::string &auth = "");
drogon::HttpResponsePtr patch(const std::string &path, const std::string &auth = "");
drogon::HttpResponsePtr del(const std::string &path, const std::string &auth = "");

}  // namespace testserver
