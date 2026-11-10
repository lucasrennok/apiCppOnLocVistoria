#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>

#include <cstdlib>
#include <string>
#include <thread>

#include "common/Config.h"
#include "common/Db.h"
#include "support/DbState.h"
#include "support/TestServer.h"

// main() dos testes de integração. O Drogon exige que app().run() rode na
// thread principal, então a aplicação sobe na main thread e os testes rodam
// em uma thread auxiliar. Sem banco, os testes são pulados (GTEST_SKIP).
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  setenv("JWT_SECRET", "0123456789abcdef0123456789abcdef", 1);
  setenv("PBKDF2_ITERATIONS", "1000", 1);

  const std::string host = common::env("DB_HOST", "127.0.0.1");
  const uint16_t port = static_cast<uint16_t>(std::stoi(common::env("DB_PORT", "3306")));
  const std::string dbname = common::env("DB_NAME", "appdb");
  const std::string user = common::env("DB_USER", "root");
  const std::string password = common::env("DB_PASSWORD", "");

  // Registra o client da aplicação. A conexão real só é criada em run().
  // Usamos UMA conexão no pool: o commit do Drogon é assíncrono (feito no
  // destrutor do Transaction), e com um pool de 1 conexão a próxima
  // transação só recebe a conexão após o commit — evitando corrida entre
  // create/update/remove encadeados nos testes.
  bool configured = false;
  try {
    drogon::app().createDbClient("mysql", host, port, dbname, user, password,
                                 1, "", "default", false, "utf8mb4");
    configured = true;
  } catch (...) {
    configured = false;
  }

  if (!configured) {
    return RUN_ALL_TESTS();
  }

  const int apiPort = std::stoi(common::env("TEST_API_PORT", "18080"));

  // Os testes rodam em uma thread auxiliar; ao final, chamam quit() para
  // liberar o loop da aplicação (que roda na thread principal).
  int rc = 0;
  std::thread testThread([&]() {
    testserver::waitReady(apiPort);

    bool dbOk = false;
    try {
      if (repo::db()) {
        repo::db()->execSqlSync("SELECT 1");
        dbOk = true;
      }
    } catch (...) {
      dbOk = false;
    }
    dbtest::setAvailable(dbOk);

    rc = RUN_ALL_TESTS();

    testserver::shutdown();
    drogon::app().quit();
  });

  testserver::run(apiPort);  // bloqueia a thread principal até quit()
  testThread.join();
  return rc;
}
