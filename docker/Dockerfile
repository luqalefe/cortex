FROM --platform=linux/arm64 debian:trixie
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config git ca-certificates \
    faust lv2-dev lilv-utils jalv libjack-jackd2-dev libasound2-dev \
    libboost-dev python3 && rm -rf /var/lib/apt/lists/*
WORKDIR /codex
