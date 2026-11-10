#include <gtest/gtest.h>
#include <drogon/drogon.h>
#include <json/json.h>

#include <cctype>
#include <string>

#include "common/Util.h"
#include "support/TestSupport.h"

TEST(UtilTest, IsoToSqlValid) {
  EXPECT_EQ(*common::isoToSql("2026-01-02T03:04:05Z"), "2026-01-02 03:04:05");
  EXPECT_EQ(*common::isoToSql("2026-01-02T03:04:05.123Z"), "2026-01-02 03:04:05");
  EXPECT_EQ(*common::isoToSql("2026-01-02 03:04:05"), "2026-01-02 03:04:05");
}

TEST(UtilTest, IsoToSqlInvalid) {
  EXPECT_FALSE(common::isoToSql("2026-13-02T03:04:05Z").has_value());
  EXPECT_FALSE(common::isoToSql("2026-01-32T03:04:05Z").has_value());
  EXPECT_FALSE(common::isoToSql("curto").has_value());
  EXPECT_FALSE(common::isoToSql("2026-01-02T03:04:05X").has_value());
}

TEST(UtilTest, EscapeLike) {
  EXPECT_EQ(common::escapeLike("a%b_c\\d"), "a\\%b\\_c\\\\d");
  EXPECT_EQ(common::escapeLike("sem-meta"), "sem-meta");
}

TEST(UtilTest, ToJsonTextRoundTrip) {
  Json::Value arr(Json::arrayValue);
  arr.append("a");
  arr.append(1);
  const std::string text = common::toJsonText(arr);
  Json::Value back = common::parseJsonText(text);
  ASSERT_TRUE(back.isArray());
  EXPECT_EQ(back.size(), 2u);
  EXPECT_EQ(back[0].asString(), "a");
}

TEST(UtilTest, ParseJsonTextInvalidReturnsEmptyArray) {
  Json::Value v = common::parseJsonText("isto não é json");
  EXPECT_TRUE(v.isArray());
  EXPECT_EQ(v.size(), 0u);
}

TEST(UtilTest, NewId) {
  const std::string id = common::newId("a");
  EXPECT_EQ(id.size(), 17u);
  EXPECT_EQ(id[0], 'a');
  for (size_t i = 1; i < id.size(); ++i)
    EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(id[i])));
}

TEST(UtilTest, NowSqlFormat) {
  const std::string s = common::nowSql();
  EXPECT_EQ(s.size(), 19u);
  EXPECT_EQ(s[4], '-');
  EXPECT_EQ(s[7], '-');
  EXPECT_EQ(s[10], ' ');
  EXPECT_EQ(s[13], ':');
  EXPECT_EQ(s[16], ':');
}

TEST(UtilTest, ReqString) {
  Json::Value o;
  o["name"] = "abc";
  EXPECT_EQ(common::reqString(o, "name"), "abc");

  EXPECT_API_ERROR(common::reqString(o, "missing"), drogon::k400BadRequest);

  o["vazio"] = "";
  EXPECT_API_ERROR(common::reqString(o, "vazio"), drogon::k400BadRequest);

  o["longo"] = std::string(300, 'x');
  EXPECT_API_ERROR(common::reqString(o, "longo", 100), drogon::k400BadRequest);
}

TEST(UtilTest, OptString) {
  Json::Value o;
  o["x"] = "abc";
  EXPECT_EQ(common::optString(o, "x").value(), "abc");
  EXPECT_FALSE(common::optString(o, "missing").has_value());

  o["nulo"] = Json::nullValue;
  EXPECT_FALSE(common::optString(o, "nulo").has_value());

  o["numero"] = 5;
  EXPECT_API_ERROR(common::optString(o, "numero"), drogon::k400BadRequest);
}

TEST(UtilTest, OneOf) {
  EXPECT_EQ(common::oneOf("a", "k", {"a", "b"}), "a");
  EXPECT_API_ERROR(common::oneOf("c", "k", {"a", "b"}), drogon::k400BadRequest);
}

TEST(UtilTest, BodyObject) {
  auto ok = drogon::HttpRequest::newHttpRequest();
  ok->setBody("{\"a\": 1}");
  ok->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  Json::Value o = common::bodyObject(ok);
  EXPECT_EQ(o["a"].asInt(), 1);

  // Cada caso usa um request novo: o Drogon faz cache do JSON parseado.
  auto arr = drogon::HttpRequest::newHttpRequest();
  arr->setBody("[1, 2]");
  arr->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  EXPECT_API_ERROR(common::bodyObject(arr), drogon::k400BadRequest);

  auto empty = drogon::HttpRequest::newHttpRequest();
  empty->setBody("");
  empty->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  EXPECT_API_ERROR(common::bodyObject(empty), drogon::k400BadRequest);
}
