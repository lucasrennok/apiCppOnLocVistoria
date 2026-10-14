#include "filters/AuthFilter.h"

#include "auth/Jwt.h"
#include "common/Http.h"

void AuthFilter::doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb,
                          drogon::FilterChainCallback &&fccb) {
  auto claims = auth::claimsFromRequest(req);
  if (!claims) {
    fcb(common::messageResponse(drogon::k401Unauthorized, "Não autenticado"));
    return;
  }
  fccb();
}
