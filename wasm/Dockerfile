FROM emscripten/emsdk:latest AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        bash \
        ca-certificates \
        cmake \
        git \
        make \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    HOME=/tmp \
    EM_CACHE=/tmp/emscripten-cache \
    EM_PORTS=/tmp/emscripten-ports

WORKDIR /src

COPY . .

RUN bash ./wasm/build-wasm.sh --deps

FROM nginx:alpine AS runtime

COPY --from=builder /src/build-wasm/pvz-portable.html /usr/share/nginx/html/index.html
COPY --from=builder \
    /src/build-wasm/pvz-portable.js \
    /src/build-wasm/pvz-portable.wasm \
    /usr/share/nginx/html/

EXPOSE 80
