#pragma once
#include <drogon/HttpFilter.h>

// Exige "Authorization: Bearer <jwt>" válido. Sem ele: 401 { "message": ... }
class AuthFilter : public drogon::HttpFilter<AuthFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb,
                drogon::FilterChainCallback &&fccb) override;
};
