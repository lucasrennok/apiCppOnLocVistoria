#pragma once

#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>

#include <string>

// Helpers para os testes de controladores. A aplicação deve ser iniciada com
// testserver::run() na THREAD PRINCIPAL (o Drogon exige run() na main thread);
// os testes rodam em outra thread usando waitReady() + os helpers HTTP.
namespace testserver {

void run(int port);
void waitReady(int port);
void shutdown();
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
