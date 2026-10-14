#pragma once
#include <string>

namespace uploads {
struct Presigned {
  std::string uploadUrl;  // PUT direto no MinIO (expira em 5 min)
  std::string fileUrl;    // URL pública de leitura
};
// Lança common::ApiError em caso de nome inválido / S3 não configurado
Presigned presign(const std::string &filename);
}  // namespace uploads
