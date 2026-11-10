#include <gtest/gtest.h>
#include <drogon/drogon.h>

#include "filters/AuthFilter.h"
#include "support/TestSupport.h"

TEST(AuthFilterTest, AllowsValidToken) {
  auto req = drogon::HttpRequest::newHttpRequest();
  req->addHeader("Authorization", testsupport::bearer("Analista"));

  bool chained = false, blocked = false;
  int status = 0;
  AuthFilter filter;
  filter.doFilter(
      req,
      [&](const drogon::HttpResponsePtr &resp) {
        blocked = true;
        status = resp->getStatusCode();
      },
      [&] { chained = true; });

  EXPECT_TRUE(chained);
  EXPECT_FALSE(blocked);
}

TEST(AuthFilterTest, RejectsMissingToken) {
  auto req = drogon::HttpRequest::newHttpRequest();

  bool chained = false, blocked = false;
  int status = 0;
  AuthFilter filter;
  filter.doFilter(
      req,
      [&](const drogon::HttpResponsePtr &resp) {
        blocked = true;
        status = resp->getStatusCode();
      },
      [&] { chained = true; });

  EXPECT_FALSE(chained);
  EXPECT_TRUE(blocked);
  EXPECT_EQ(status, drogon::k401Unauthorized);
}

TEST(AuthFilterTest, RejectsInvalidToken) {
  auto req = drogon::HttpRequest::newHttpRequest();
  req->addHeader("Authorization", "Bearer token.invalido");

  bool chained = false, blocked = false;
  int status = 0;
  AuthFilter filter;
  filter.doFilter(
      req,
      [&](const drogon::HttpResponsePtr &resp) {
        blocked = true;
        status = resp->getStatusCode();
      },
      [&] { chained = true; });

  EXPECT_FALSE(chained);
  EXPECT_TRUE(blocked);
  EXPECT_EQ(status, drogon::k401Unauthorized);
}
