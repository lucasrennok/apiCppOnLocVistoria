#include <gtest/gtest.h>
#include <json/json.h>

#include <cstdlib>

#include "support/TestSupport.h"

class UploadControllerTest : public ApiTestBase {
 protected:
  void SetUp() override {
    ApiTestBase::SetUp();
    unsetenv("S3_ACCESS_KEY");
    unsetenv("S3_SECRET_KEY");
  }
  void TearDown() override {
    unsetenv("S3_ACCESS_KEY");
    unsetenv("S3_SECRET_KEY");
  }
};

TEST_F(UploadControllerTest, PresignWithoutStorageReturns503) {
  Json::Value body;
  body["filename"] = "foto.png";
  auto resp = testserver::post("/uploads/presign", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k503ServiceUnavailable);
}

TEST_F(UploadControllerTest, PresignWithStorageReturnsUrls) {
  setenv("S3_ACCESS_KEY", "AKTEST", 1);
  setenv("S3_SECRET_KEY", "SKTEST", 1);
  setenv("S3_PUBLIC_ENDPOINT", "http://localhost:9000", 1);

  Json::Value body;
  body["filename"] = "foto.png";
  auto resp = testserver::post("/uploads/presign", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k200OK);
  Json::Value out = testsupport::parseBody(resp);
  EXPECT_NE(out["uploadUrl"].asString().find("X-Amz-Signature="), std::string::npos);
  EXPECT_EQ(out["fileUrl"].asString().find("http://localhost:9000/onloc/"), 0u);
}

TEST_F(UploadControllerTest, PresignInvalidFilenameReturns400) {
  setenv("S3_ACCESS_KEY", "AKTEST", 1);
  setenv("S3_SECRET_KEY", "SKTEST", 1);

  Json::Value body;
  body["filename"] = "///";
  auto resp = testserver::post("/uploads/presign", body);
  ASSERT_NE(resp, nullptr);
  EXPECT_EQ(resp->getStatusCode(), drogon::k400BadRequest);
}
