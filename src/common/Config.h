#pragma once
#include <cstdlib>
#include <string>

namespace common {

inline std::string env(const char *key, const std::string &fallback = "") {
  const char *v = std::getenv(key);
  return (v && *v) ? std::string(v) : fallback;
}

inline const std::string &jwtSecret() {
  static const std::string s = env("JWT_SECRET");
  return s;
}

inline int pbkdf2Iterations() {
  static const int n = [] {
    try {
      return std::stoi(env("PBKDF2_ITERATIONS", "210000"));
    } catch (...) {
      return 210000;
    }
  }();
  return n;
}

}  // namespace common
