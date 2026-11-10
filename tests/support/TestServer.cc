#include "support/TestServer.h"

#include <chrono>
#include <thread>

#include "common/Util.h"

namespace testserver {
namespace {

std::thread g_thread;
drogon::HttpClientPtr g_client;
bool g_ready = false;

std::string jsonText(const Json::Value &v) { return common::toJsonText(v); }

}  // namespace

void start(int port) {
  drogon::app().addListener("127.0.0.1", static_cast<uint16_t>(port));
  g_thread = std::thread([] { drogon::app().run(); });
  g_client = drogon::HttpClient::newHttpClient("http://127.0.0.1:" + std::to_string(port));

  // Aguarda o servidor ficar pronto (o /health responde 200 quando o banco está OK).
  for (int i = 0; i < 100; ++i) {
    try {
      auto req = drogon::HttpRequest::newHttpRequest();
      req->setPath("/health");
      auto [result, resp] = g_client->sendRequest(req, 0.5);
      if (result == drogon::ReqResult::Ok && resp &&
          resp->getStatusCode() != drogon::k503ServiceUnavailable) {
        g_ready = true;
        return;
      }
    } catch (...) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void stop() {
  if (g_thread.joinable()) {
    drogon::app().quit();
    g_thread.join();
  }
  g_ready = false;
}

bool running() { return g_ready; }

drogon::HttpResponsePtr request(drogon::HttpMethod method, const std::string &path,
                                const std::string &body, const std::string &authHeader) {
  if (!g_ready) return nullptr;

  auto req = drogon::HttpRequest::newHttpRequest();
  req->setMethod(method);
  req->setPath(path);
  if (!body.empty()) {
    req->setBody(body);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  }
  if (!authHeader.empty()) req->addHeader("Authorization", authHeader);

  auto [result, resp] = g_client->sendRequest(req, 5.0);
  if (result != drogon::ReqResult::Ok) return nullptr;
  return resp;
}

drogon::HttpResponsePtr get(const std::string &path, const std::string &auth) {
  return request(drogon::Get, path, "", auth);
}

drogon::HttpResponsePtr post(const std::string &path, const Json::Value &body,
                             const std::string &auth) {
  return request(drogon::Post, path, jsonText(body), auth);
}

drogon::HttpResponsePtr put(const std::string &path, const Json::Value &body,
                            const std::string &auth) {
  return request(drogon::Put, path, jsonText(body), auth);
}

drogon::HttpResponsePtr patch(const std::string &path, const std::string &auth) {
  return request(drogon::Patch, path, "", auth);
}

drogon::HttpResponsePtr del(const std::string &path, const std::string &auth) {
  return request(drogon::Delete, path, "", auth);
}

}  // namespace testserver
