#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "services/UploadService.h"
#include "support/TestSupport.h"

namespace {
void configureS3() {
  setenv("S3_ACCESS_KEY", "AKTEST", 1);
  setenv("S3_SECRET_KEY", "SKTEST", 1);
  setenv("S3_PUBLIC_ENDPOINT", "http://localhost:9000", 1);
  setenv("S3_BUCKET", "onloc", 1);
  setenv("S3_REGION", "us-east-1", 1);
}
}  // namespace

class UploadServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    unsetenv("S3_ACCESS_KEY");
    unsetenv("S3_SECRET_KEY");
  }
  void TearDown() override {
    unsetenv("S3_ACCESS_KEY");
    unsetenv("S3_SECRET_KEY");
  }
};

TEST_F(UploadServiceTest, ThrowsWhenNotConfigured) {
  EXPECT_API_ERROR(uploads::presign("foto.png"), drogon::k503ServiceUnavailable);
}

TEST_F(UploadServiceTest, ThrowsOnInvalidFilename) {
  configureS3();
  EXPECT_API_ERROR(uploads::presign("///"), drogon::k400BadRequest);
  EXPECT_API_ERROR(uploads::presign("..."), drogon::k400BadRequest);
}

TEST_F(UploadServiceTest, GeneratesPresignedUrl) {
  configureS3();
  const uploads::Presigned p = uploads::presign("foto.png");

  EXPECT_EQ(p.fileUrl.find("http://localhost:9000/onloc/"), 0u);
  EXPECT_NE(p.uploadUrl.find("http://localhost:9000/onloc/"), std::string::npos);
  EXPECT_NE(p.uploadUrl.find("X-Amz-Algorithm=AWS4-HMAC-SHA256"), std::string::npos);
  EXPECT_NE(p.uploadUrl.find("X-Amz-Expires=300"), std::string::npos);
  EXPECT_NE(p.uploadUrl.find("X-Amz-Signature="), std::string::npos);
}

TEST_F(UploadServiceTest, SanitizesFilename) {
  configureS3();
  const uploads::Presigned p = uploads::presign("../dir/foto foto.png");
  EXPECT_EQ(p.uploadUrl.find(".."), std::string::npos);
  EXPECT_EQ(p.uploadUrl.find(' '), std::string::npos);
  EXPECT_NE(p.uploadUrl.find("foto_foto.png"), std::string::npos);
}
