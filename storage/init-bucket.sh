#!/bin/sh
# Cria o bucket e libera leitura pública dos objetos (as fotos são exibidas por URL direta).
set -e
export AWS_ACCESS_KEY_ID="$S3_ACCESS_KEY"
export AWS_SECRET_ACCESS_KEY="$S3_SECRET_KEY"
export AWS_DEFAULT_REGION="${S3_REGION:-us-east-1}"
EP="http://storage:9000"

echo "Aguardando o storage..."
until aws --endpoint-url "$EP" s3api list-buckets >/dev/null 2>&1; do sleep 2; done

aws --endpoint-url "$EP" s3api head-bucket --bucket "$S3_BUCKET" 2>/dev/null \
  || aws --endpoint-url "$EP" s3api create-bucket --bucket "$S3_BUCKET"

cat > /tmp/policy.json <<POLICY
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Principal": "*",
    "Action": ["s3:GetObject"],
    "Resource": ["arn:aws:s3:::${S3_BUCKET}/*"]
  }]
}
POLICY
aws --endpoint-url "$EP" s3api put-bucket-policy --bucket "$S3_BUCKET" --policy file:///tmp/policy.json
# CORS: o navegador (outra origem) faz PUT direto no bucket (best effort)
aws --endpoint-url "$EP" s3api put-bucket-cors --bucket "$S3_BUCKET" --cors-configuration '{"CORSRules":[{"AllowedOrigins":["*"],"AllowedMethods":["GET","PUT","HEAD"],"AllowedHeaders":["*"],"ExposeHeaders":["ETag"],"MaxAgeSeconds":3000}]}' \
  || echo "Aviso: não foi possível configurar CORS no bucket"
echo "Bucket '$S3_BUCKET' pronto."
