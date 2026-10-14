# On.loc API (C++ / Drogon / MySQL)

Implementa o contrato de `API-MOCK.md`.

## Subir
    cp .env.example .env
    # preencha JWT_SECRET (openssl rand -hex 32) e SEED_DEFAULT_PASSWORD (só dev)
    docker compose up --build                         # API + MySQL
docker compose --profile storage up --build       # + storage de fotos (RustFS)
O 1º build demora (compila o Drogon). API em http://localhost:8080; storage em :9000 (console :9001).

## Login de teste (com SEED_DEFAULT_PASSWORD definido)
    curl -X POST localhost:8080/api/auth/login -H 'Content-Type: application/json' \
         -d '{"email":"analista@onloc.com.br","password":"<SEED_DEFAULT_PASSWORD>"}'
    TOKEN=<token retornado>
    curl localhost:8080/api/users -H "Authorization: Bearer $TOKEN"
    curl localhost:8080/api/clients?search=alfa
    curl -X PATCH localhost:8080/api/inspections/i2/approve -H "Authorization: Bearer $TOKEN"

As rotas respondem com ou sem o prefixo `/api` (nginx pode repassar de qualquer forma).

## Estrutura
- src/main.cc            bootstrap (porta, pool MySQL, prefixo /api, seed de senhas)
- src/controllers/       rotas HTTP (uma classe por recurso)
- src/filters/           AuthFilter (JWT Bearer -> 401)
- src/repositories/      acesso ao banco (SQL parametrizado, transações)
- src/services/          presign S3/MinIO (AWS SigV4)
- src/auth, src/common/  JWT HS256, PBKDF2, validações, helpers
- db/init.sql            schema + seed do documento

## Notas
- IDs novos: prefixo + 16 hex (`c…`, `a…`, `l…`, `li…`, `i…`, `d…`, `u…`, `s…`). Seed mantém `u1`, `c1`…
- Erros extras além de 401/404: 400 (validação), 409 (conflito: e-mail duplicado, excluir com vínculos), 503 (S3 não configurado).
- `POST /users` aceita `password` opcional (mín. 8). Sem ela o usuário não consegue logar.
- Upload: `uploadUrl` é um PUT pré-assinado (5 min) direto no storage S3 (RustFS local); `fileUrl` é leitura pública do bucket.
  Atrás de proxy, ajuste `S3_PUBLIC_ENDPOINT` para a URL que o navegador usa (o host entra na assinatura).

## ORM do Drogon (opcional)
Hoje o acesso a dados usa SQL parametrizado (`?`). Para gerar modelos do ORM:
    docker compose up -d db
    docker compose run --rm --no-deps -v "$PWD/models:/app/models" --entrypoint drogon_ctl api create model /app/models
