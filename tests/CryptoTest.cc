#include <gtest/gtest.h>

#include <string>

#include "common/Crypto.h"

TEST(CryptoTest, HexEncode) {
  EXPECT_EQ(crypto::hexEncode(""), "");
  EXPECT_EQ(crypto::hexEncode("abc"), "616263");
  EXPECT_EQ(crypto::hexEncode(std::string("\x00\xff", 2)), "00ff");
}

TEST(CryptoTest, HexDecode) {
  std::string out;
  EXPECT_TRUE(crypto::hexDecode("616263", out));
  EXPECT_EQ(out, "abc");

  EXPECT_FALSE(crypto::hexDecode("abc", out));  // comprimento ímpar
  EXPECT_FALSE(crypto::hexDecode("6g", out));   // caractere inválido

  EXPECT_TRUE(crypto::hexDecode("", out));
  EXPECT_EQ(out, "");
}

TEST(CryptoTest, Sha256Hex) {
  EXPECT_EQ(crypto::sha256Hex(""),
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  EXPECT_EQ(crypto::sha256Hex("abc"),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(CryptoTest, HmacSha256KnownVector) {
  // Vetor de teste RFC 4231.
  EXPECT_EQ(crypto::hexEncode(crypto::hmacSha256(
                "key", "The quick brown fox jumps over the lazy dog")),
            "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
}

TEST(CryptoTest, ConstantTimeEquals) {
  EXPECT_TRUE(crypto::constantTimeEquals("abc", "abc"));
  EXPECT_FALSE(crypto::constantTimeEquals("abc", "abd"));
  EXPECT_FALSE(crypto::constantTimeEquals("abc", "ab"));
}

TEST(CryptoTest, Base64UrlEncode) {
  EXPECT_EQ(crypto::base64UrlEncode(""), "");
  EXPECT_EQ(crypto::base64UrlEncode("f"), "Zg");
  EXPECT_EQ(crypto::base64UrlEncode("fo"), "Zm8");
  EXPECT_EQ(crypto::base64UrlEncode("foo"), "Zm9v");
}

TEST(CryptoTest, Base64UrlRoundTrip) {
  const std::string in = "texto com espaços, acentos áéíóú e símbolos ~.-_";
  const std::string enc = crypto::base64UrlEncode(in);
  std::string out;
  ASSERT_TRUE(crypto::base64UrlDecode(enc, out));
  EXPECT_EQ(out, in);
}

TEST(CryptoTest, Base64UrlDecodeRejectsInvalid) {
  std::string out;
  EXPECT_FALSE(crypto::base64UrlDecode("!!!!", out));
}

TEST(CryptoTest, HashAndVerifyPassword) {
  const std::string hash = crypto::hashPassword("senha-secreta", 1000);
  EXPECT_EQ(hash.rfind("pbkdf2$1000$", 0), 0u);

  EXPECT_TRUE(crypto::verifyPassword("senha-secreta", hash));
  EXPECT_FALSE(crypto::verifyPassword("senha-errada", hash));
  EXPECT_FALSE(crypto::verifyPassword("senha-secreta", "formato-invalido"));
  EXPECT_FALSE(crypto::verifyPassword("", hash));
}
