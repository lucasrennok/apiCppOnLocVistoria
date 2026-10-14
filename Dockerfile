# ---------- Stage 1: compila Drogon + a aplicação ----------
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    git ca-certificates cmake g++ make pkg-config \
    libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev \
    libmariadb-dev libc-ares-dev \
 && rm -rf /var/lib/apt/lists/*

# Drogon (framework web + ORM) — compila com suporte a MySQL automaticamente
RUN git clone --branch v1.9.10 --depth 1 --recurse-submodules --shallow-submodules \
      https://github.com/drogonframework/drogon.git /tmp/drogon \
 && cmake -S /tmp/drogon -B /tmp/drogon/build \
      -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF \
 && cmake --build /tmp/drogon/build -j"$(nproc)" \
 && cmake --install /tmp/drogon/build \
 && rm -rf /tmp/drogon

WORKDIR /src
COPY CMakeLists.txt .
COPY src ./src
COPY models ./models
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j"$(nproc)"

# ---------- Stage 2: imagem final enxuta ----------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libjsoncpp25 libmariadb3 libssl3 zlib1g libuuid1 libc-ares2 ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --system --no-create-home appuser

WORKDIR /app
COPY --from=build /src/build/api_cpp /app/api_cpp
# drogon_ctl: usado só para gerar os modelos ORM (ver README)
COPY --from=build /usr/local/bin/drogon_ctl /usr/local/bin/drogon_ctl

RUN mkdir -p /app/uploads/tmp \ 
&& chown -R appuser:appuser /app/uploads

USER appuser
EXPOSE 8080
ENTRYPOINT ["/app/api_cpp"]
