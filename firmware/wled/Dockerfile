# Build WLED for the PDX Hackerspace Hackerbadge (Xiao ESP32-S3).

FROM node:20-bookworm-slim

ARG WLED_VERSION=v16.0.1

ENV DEBIAN_FRONTEND=noninteractive \
    WLED_VERSION=${WLED_VERSION} \
    PIO_ENV=hackerbadge \
    PLATFORMIO_CORE_DIR=/cache/platformio \
    npm_config_cache=/cache/npm

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        git \
        python3 \
        python3-pip \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/pio \
    && /opt/pio/bin/pip install --no-cache-dir -U pip \
    && /opt/pio/bin/pip install --no-cache-dir platformio \
    && ln -sf /opt/pio/bin/pio /usr/local/bin/pio \
    && ln -sf /opt/pio/bin/platformio /usr/local/bin/platformio \
    && pio --version

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

WORKDIR /wled
VOLUME ["/wled", "/build", "/cache", "/override"]

ENTRYPOINT ["/entrypoint.sh"]
CMD ["compile"]
