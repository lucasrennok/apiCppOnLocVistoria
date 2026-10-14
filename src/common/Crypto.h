#pragma once
#include <string>

namespace crypto {

std::string hexEncode(const std::string &raw);
bool hexDecode(const std::string &hex, std::string &out);
std::string sha256Hex(const std::string &data);
std::string hmacSha256(const std::string &key, const std::string &data);  // bytes brutos
bool constantTimeEquals(const std::string &a, const std::string &b);

std::string base64UrlEncode(const std::string &in);
bool base64UrlDecode(const std::string &in, std::string &out);

// Formato: pbkdf2$<iterações>$<salt hex>$<hash hex>
std::string hashPassword(const std::string &password, int iterations);
bool verifyPassword(const std::string &password, const std::string &stored);

}  // namespace crypto
