cmake_minimum_required(VERSION 3.16)
project(api_cpp CXX)

# C++20: necessário para coroutines (co_await) usadas nos handlers/repositórios
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE APP_SRCS CONFIGURE_DEPENDS src/*.cc)
file(GLOB MODEL_SRCS CONFIGURE_DEPENDS models/*.cc)

# main.cc define a função main(); os demais arquivos viram uma biblioteca de
# objetos para que a aplicação e os testes compartilhem o mesmo código.
set(APP_LIB_SRCS ${APP_SRCS})
list(FILTER APP_LIB_SRCS EXCLUDE REGEX "(^|/)main[.]cc$")

add_library(api_cpp_core OBJECT ${APP_LIB_SRCS} ${MODEL_SRCS})
target_include_directories(api_cpp_core PUBLIC src models)
target_link_libraries(api_cpp_core PUBLIC Drogon::Drogon OpenSSL::Crypto)

add_executable(${PROJECT_NAME} src/main.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE api_cpp_core)

# Testes (desabilitados por padrão para não afetar o build do Docker).
option(BUILD_TESTING "Compilar e habilitar os testes" OFF)

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)
endif()
