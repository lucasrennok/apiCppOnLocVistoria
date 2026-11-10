# Testes unitários (não dependem do banco de dados).
set(UNIT_TEST_SRCS
  unit_main.cc
  CryptoTest.cc
  JwtTest.cc
  UtilTest.cc
  ConfigTest.cc
  HttpHelpersTest.cc
  UploadServiceTest.cc
  AuthFilterTest.cc
  GestorFilterTest.cc
)

# Testes de integração (dependem do MySQL; são pulados se ele não existir).
set(INTEGRATION_TEST_SRCS
  integration_main.cc
  support/DbState.cc
  support/TestServer.cc
  AssetRepositoryTest.cc
  ClientRepositoryTest.cc
  LoanRepositoryTest.cc
  InspectionRepositoryTest.cc
  UserRepositoryTest.cc
  AssetControllerTest.cc
  AuthControllerTest.cc
  ClientControllerTest.cc
  HelloControllerTest.cc
  InspectionControllerTest.cc
  LoanControllerTest.cc
  UploadControllerTest.cc
  UserControllerTest.cc
  MainTest.cc
)

add_executable(unit_tests ${UNIT_TEST_SRCS})
target_link_libraries(unit_tests PRIVATE api_cpp_core GTest::gtest)
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(integration_tests ${INTEGRATION_TEST_SRCS})
target_link_libraries(integration_tests PRIVATE api_cpp_core GTest::gtest)
target_include_directories(integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Evita ruído de warnings do gtest/googletest em compiladores mais recentes.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(unit_tests PRIVATE -w)
  target_compile_options(integration_tests PRIVATE -w)
endif()

add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME integration_tests COMMAND integration_tests)
