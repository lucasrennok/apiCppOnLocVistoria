#include "filters/GestorFilter.h"

#include "auth/Jwt.h"
#include "common/Http.h"

void GestorFilter::doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb,
                            drogon::FilterChainCallback &&fccb) {
  auto claims = auth::claimsFromRequest(req);
  if (!claims) {
    fcb(common::messageResponse(drogon::k401Unauthorized, "Não autenticado"));
    return;
  }
  if ((*claims)["role"].asString() != "Gestor") {
    fcb(common::messageResponse(drogon::k403Forbidden, "Acesso restrito ao perfil Gestor"));
    return;
  }
  fccb();
}
