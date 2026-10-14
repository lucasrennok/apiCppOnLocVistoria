#pragma once
#include <drogon/HttpFilter.h>

// Exige JWT válido (401) e papel "Gestor" (403). Usado em aprovação/reprovação e em /users.
class GestorFilter : public drogon::HttpFilter<GestorFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb,
                drogon::FilterChainCallback &&fccb) override;
};
