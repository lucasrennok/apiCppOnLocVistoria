cmake_minimum_required(VERSION 3.16)
project(api_cpp CXX)

# C++20: necessário para coroutines (co_await) usadas nos handlers/repositórios
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE APP_SRCS CONFIGURE_DEPENDS src/*.cc)
file(GLOB MODEL_SRCS CONFIGURE_DEPENDS models/*.cc)

add_executable(${PROJECT_NAME} ${APP_SRCS} ${MODEL_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE src models)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon OpenSSL::Crypto)
