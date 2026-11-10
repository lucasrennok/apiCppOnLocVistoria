#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>

#include <cstdlib>
#include <string>

#include "common/Config.h"
#include "common/Db.h"
#include "support/DbState.h"
#include "support/TestServer.h"

// main() dos testes de integração. Conecta no MySQL e, se o banco estiver
// disponível, sobe a aplicação HTTP em uma thread para os testes de
// controladores. Sem banco, os testes simplesmente são pulados (GTEST_SKIP).
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  setenv("JWT_SECRET", "0123456789abcdef0123456789abcdef", 1);
  setenv("PBKDF2_ITERATIONS", "1000", 1);

  const std::string host = common::env("DB_HOST", "127.0.0.1");
  const uint16_t port = static_cast<uint16_t>(std::stoi(common::env("DB_PORT", "3306")));
  const std::string dbname = common::env("DB_NAME", "appdb");
  const std::string user = common::env("DB_USER", "root");
  const std::string password = common::env("DB_PASSWORD", "");

  bool dbOk = false;
  try {
    drogon::app().createDbClient("mysql", host, port, dbname, user, password,
                                 4, "", "default", false, "utf8mb4");
    if (repo::db()) {
      drogon::sync_wait(repo::db()->execSqlCoro("SELECT 1"));
      dbOk = true;
    }
  } catch (...) {
    dbOk = false;
  }

  dbtest::setAvailable(dbOk);

  if (dbOk) {
    const int apiPort = std::stoi(common::env("TEST_API_PORT", "18080"));
    testserver::start(apiPort);
  }

  const int rc = RUN_ALL_TESTS();

  if (dbOk) testserver::stop();
  return rc;
}
