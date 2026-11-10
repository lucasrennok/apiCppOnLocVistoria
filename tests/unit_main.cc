#include <gtest/gtest.h>

#include <cstdlib>

// main() dos testes unitários (sem banco). Define variáveis de ambiente
// usadas pelas funções que dependem de configuração (JWT, PBKDF2).
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  setenv("JWT_SECRET", "0123456789abcdef0123456789abcdef", 1);
  setenv("PBKDF2_ITERATIONS", "1000", 1);
  unsetenv("S3_ACCESS_KEY");
  unsetenv("S3_SECRET_KEY");

  return RUN_ALL_TESTS();
}
