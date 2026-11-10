#include <gtest/gtest.h>
#include <drogon/drogon.h>

#include "filters/GestorFilter.h"
#include "support/TestSupport.h"

namespace {

struct Result {
  bool chained = false;
  bool blocked = false;
  int status = 0;
};

Result run(const std::string &authHeader) {
  auto req = drogon::HttpRequest::newHttpRequest();
  if (!authHeader.empty()) req->addHeader("Authorization", authHeader);

  Result r;
  GestorFilter filter;
  filter.doFilter(
      req,
      [&](const drogon::HttpResponsePtr &resp) {
        r.blocked = true;
        r.status = resp->getStatusCode();
      },
      [&] { r.chained = true; });
  return r;
}

}  // namespace

TEST(GestorFilterTest, AllowsGestor) {
  Result r = run(testsupport::bearer("Gestor"));
  EXPECT_TRUE(r.chained);
  EXPECT_FALSE(r.blocked);
}

TEST(GestorFilterTest, BlocksAnalista) {
  Result r = run(testsupport::bearer("Analista"));
  EXPECT_FALSE(r.chained);
  EXPECT_TRUE(r.blocked);
  EXPECT_EQ(r.status, drogon::k403Forbidden);
}

TEST(GestorFilterTest, BlocksWithoutToken) {
  Result r = run("");
  EXPECT_FALSE(r.chained);
  EXPECT_TRUE(r.blocked);
  EXPECT_EQ(r.status, drogon::k401Unauthorized);
}
