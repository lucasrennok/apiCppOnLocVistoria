#include "common/Crypto.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace crypto {

std::string hexEncode(const std::string &raw) {
  static const char *H = "0123456789abcdef";
  std::string out;
  out.reserve(raw.size() * 2);
  for (unsigned char c : raw) {
    out.push_back(H[c >> 4]);
    out.push_back(H[c & 0x0F]);
  }
  return out;
}

bool hexDecode(const std::string &hex, std::string &out) {
  if (hex.size() % 2) return false;
  auto val = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  out.clear();
  for (size_t i = 0; i < hex.size(); i += 2) {
    int a = val(hex[i]), b = val(hex[i + 1]);
    if (a < 0 || b < 0) return false;
    out.push_back(static_cast<char>((a << 4) | b));
  }
  return true;
}

std::string sha256Hex(const std::string &data) {
  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), h);
  return hexEncode(std::string(reinterpret_cast<char *>(h), sizeof h));
}

std::string hmacSha256(const std::string &key, const std::string &data) {
  unsigned char out[EVP_MAX_MD_SIZE];
  unsigned int len = 0;
  HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char *>(data.data()), data.size(), out, &len);
  return std::string(reinterpret_cast<char *>(out), len);
}

bool constantTimeEquals(const std::string &a, const std::string &b) {
  if (a.size() != b.size()) return false;
  return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

static const char *kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string base64UrlEncode(const std::string &in) {
  std::string out;
  uint32_t val = 0;
  int valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(kB64[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) out.push_back(kB64[((val << 8) >> (valb + 8)) & 0x3F]);
  return out;  // sem padding
}

bool base64UrlDecode(const std::string &in, std::string &out) {
  static const std::array<int8_t, 256> table = [] {
    std::array<int8_t, 256> t;
    t.fill(-1);
    for (int i = 0; i < 64; ++i) t[static_cast<unsigned char>(kB64[i])] = static_cast<int8_t>(i);
    return t;
  }();
  out.clear();
  uint32_t val = 0;
  int valb = -8;
  for (unsigned char c : in) {
    if (c == '=') break;
    int d = table[c];
    if (d < 0) return false;
    val = (val << 6) + static_cast<uint32_t>(d);
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<char>((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return true;
}

std::string hashPassword(const std::string &password, int iterations) {
  unsigned char salt[16];
  if (RAND_bytes(salt, sizeof salt) != 1) throw std::runtime_error("RAND_bytes falhou");
  unsigned char out[32];
  if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt, sizeof salt,
                        iterations, EVP_sha256(), sizeof out, out) != 1)
    throw std::runtime_error("PBKDF2 falhou");
  return "pbkdf2$" + std::to_string(iterations) + "$" +
         hexEncode(std::string(reinterpret_cast<char *>(salt), sizeof salt)) + "$" +
         hexEncode(std::string(reinterpret_cast<char *>(out), sizeof out));
}

bool verifyPassword(const std::string &password, const std::string &stored) {
  std::vector<std::string> parts;
  size_t start = 0, pos;
  while ((pos = stored.find('$', start)) != std::string::npos) {
    parts.push_back(stored.substr(start, pos - start));
    start = pos + 1;
  }
  parts.push_back(stored.substr(start));
  if (parts.size() != 4 || parts[0] != "pbkdf2") return false;

  int iterations = 0;
  try {
    iterations = std::stoi(parts[1]);
  } catch (...) {
    return false;
  }
  std::string salt, expected;
  if (iterations < 1 || !hexDecode(parts[2], salt) || !hexDecode(parts[3], expected) ||
      expected.empty())
    return false;

  std::string out(expected.size(), '\0');
  if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                        reinterpret_cast<const unsigned char *>(salt.data()),
                        static_cast<int>(salt.size()), iterations, EVP_sha256(),
                        static_cast<int>(out.size()),
                        reinterpret_cast<unsigned char *>(out.data())) != 1)
    return false;
  return constantTimeEquals(out, expected);
}

}  // namespace crypto
