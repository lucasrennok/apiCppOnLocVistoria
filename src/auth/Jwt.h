#pragma once
#include <drogon/HttpRequest.h>
#include <json/json.h>

#include <optional>
#include <string>

namespace auth {

// JWT HS256: header.payload.signature (base64url)
std::string sign(const Json::Value &payload, const std::string &secret);

// Valida assinatura, algoritmo e expiração (exp). Retorna o payload ou nullopt.
std::optional<Json::Value> verify(const std::string &token, const std::string &secret);

// Lê "Authorization: Bearer <token>" e valida
std::optional<Json::Value> claimsFromRequest(const drogon::HttpRequestPtr &req);

}  // namespace auth
