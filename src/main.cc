#include <drogon/drogon.h>

#include "common/Config.h"
#include "common/Crypto.h"
#include "common/Db.h"

using namespace drogon;

// Em DEV: define a senha dos usuários do seed (u1, u2) a partir de SEED_DEFAULT_PASSWORD.
// Só atua em quem ainda não tem senha. Remova a variável em produção.
static Task<> seedPasswords(std::string password) {
  try {
    auto c = repo::db();
    auto r = co_await c->execSqlCoro(
        "SELECT id FROM users WHERE password_hash IS NULL AND id IN ('u1', 'u2')");
    for (const auto &row : r) {
      std::string id = row["id"].as<std::string>();
      std::string hash = crypto::hashPassword(password, common::pbkdf2Iterations());
      co_await c->execSqlCoro("UPDATE users SET password_hash = ? WHERE id = ?", hash, id);
      LOG_INFO << "Senha inicial definida para o usuário de seed " << id;
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Falha ao definir senhas do seed: " << e.what();
  }
}

int main() {
  if (common::jwtSecret().size() < 32) {
    LOG_FATAL << "JWT_SECRET ausente ou curto demais (mínimo 32 caracteres). "
                 "Gere um com: openssl rand -hex 32";
    return 1;
  }

  app().setLogLevel(trantor::Logger::kInfo);
  app().addListener("0.0.0.0", static_cast<uint16_t>(std::stoi(common::env("API_PORT", "8080"))));

  // O contrato usa base URL "/api". Aceitamos com ou sem o prefixo, assim funciona
  // tanto se o nginx repassar "/api/..." quanto se ele remover o prefixo.
  app().registerPreRoutingAdvice([](const HttpRequestPtr &req, AdviceCallback &&,
                                    AdviceChainCallback &&next) {
    const std::string path = req->path();
    if (path == "/api" || path.rfind("/api/", 0) == 0) {
      std::string stripped = path.substr(4);
      req->setPath(stripped.empty() ? "/" : stripped);
    }
    next();
  });

  app().createDbClient("mysql", common::env("DB_HOST", "db"),
                       static_cast<uint16_t>(std::stoi(common::env("DB_PORT", "3306"))),
                       common::env("DB_NAME", "appdb"), common::env("DB_USER", "app"),
                       common::env("DB_PASSWORD", ""),
                       4,          // conexões no pool
                       "",         // arquivo de client (não usado)
                       "default",  // nome do client
                       false,      // isFast
                       "utf8mb4");

  std::string seedPw = common::env("SEED_DEFAULT_PASSWORD");
  if (!seedPw.empty()) {
    app().registerBeginningAdvice([seedPw] {
      app().getLoop()->runAfter(2.0, [seedPw] { async_run([seedPw] { return seedPasswords(seedPw); }); });
    });
  }

  LOG_INFO << "API iniciando...";
  app().run();
  return 0;
}
