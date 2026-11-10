#include <gtest/gtest.h>

#include <string>

#include "common/Config.h"

// As variáveis JWT_SECRET e PBKDF2_ITERATIONS são definidas no main() dos testes.

TEST(ConfigTest, EnvFallback) {
  EXPECT_EQ(common::env("VARIAVEL_QUE_NAO_EXISTE_XYZ", "padrao"), "padrao");
  EXPECT_EQ(common::env("VARIAVEL_QUE_NAO_EXISTE_XYZ"), "");
}

TEST(ConfigTest, JwtSecretFromEnv) {
  EXPECT_EQ(common::jwtSecret(), "0123456789abcdef0123456789abcdef");
}

TEST(ConfigTest, Pbkdf2IterationsFromEnv) {
  EXPECT_EQ(common::pbkdf2Iterations(), 1000);
}
