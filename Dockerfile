# ═══════════════════════════════════════════════════════════════════════
# Dockerfile — Multi-stage build for Calc Engine
#
# Produces:
#   - Scratch container (~2MB, fully static binary)
#   - WASM bundle (via Emscripten stage)
#
# Build:
#   docker build -t calc-engine:latest .
#   docker run --rm calc-engine:latest --cli
#
# THE FLEX: A 2MB container running a calculator that originated
# as a bare-metal OS kernel. Containers are just lightweight VMs. 💀
# ═══════════════════════════════════════════════════════════════════════

# ── Stage 1: Build environment ──
FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    gcc-multilib \
    nasm \
    git \
    cmake \
    libsdl2-dev \
    curl \
    python3 \
    nodejs \
    && rm -rf /var/lib/apt/lists/*

# Copy source
COPY . /src
WORKDIR /src

# ── Stage 2: Desktop host binary (statically linked) ──
FROM builder AS desktop_builder
RUN make app_static CC=gcc
RUN ls -lh calc_static

# ── Stage 3: WASM target (requires emscripten) ──
FROM builder AS wasm_builder
RUN curl -L -O https://github.com/emscripten-core/emsdk/archive/master.tar.gz && \
    tar -xzf master.tar.gz && \
    cd emsdk-master && \
    ./emsdk install latest && \
    ./emsdk activate latest
RUN cd /src && \
    . /emsdk-master/emsdk_env.sh && \
    make app_wasm
RUN ls -lh /src/calc_web.wasm

# ── Stage 4: Production scratch container ──
# A completely blank container with ZERO system libraries.
# The binary is statically linked — it IS the OS as far as this
# container is concerned. No libc, no shell, no nothing. 💀
FROM scratch AS production_container
COPY --from=desktop_builder /src/calc_static /calc_service
ENTRYPOINT ["/calc_service", "--cli"]
LABEL description="Calc Engine — Bare Metal Calculator in a Scratch Container"
LABEL version="1.0"

# ── Stage 5: Minimal Alpine runner (for development) ──
FROM alpine:3.20 AS development
RUN apk add --no-cache libgcc libstdc++
COPY --from=desktop_builder /src/calc_static /usr/bin/calc
ENTRYPOINT ["calc", "--cli"]
CMD ["--cli"]
