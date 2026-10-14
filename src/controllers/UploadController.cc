#include <drogon/HttpController.h>

#include "common/Http.h"
#include "common/Util.h"
#include "services/UploadService.h"

using namespace drogon;

class UploadController : public HttpController<UploadController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UploadController::presign, "/uploads/presign", Post);
  METHOD_LIST_END

  void presign(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
      Json::Value body = common::bodyObject(req);
      auto p = uploads::presign(common::reqString(body, "filename", 255));
      Json::Value out;
      out["uploadUrl"] = p.uploadUrl;
      out["fileUrl"] = p.fileUrl;
      callback(common::json(out));
    } catch (...) {
      callback(common::errorResponse(std::current_exception()));
    }
  }
};
