# Build environment for the Hackerbadge Arduino / FastLED test sketch.
# Installs Arduino CLI, the ESP32 core (XIAO ESP32S3), and sketch libraries.

FROM debian:bookworm-slim

ARG ARDUINO_CLI_VERSION=1.2.0
ARG ESP32_CORE_VERSION=3.1.1

ENV DEBIAN_FRONTEND=noninteractive \
    ARDUINO_DIRECTORIES_DATA=/data \
    ARDUINO_DIRECTORIES_USER=/data/user \
    ARDUINO_DIRECTORIES_DOWNLOADS=/data/staging \
    FQBN=esp32:esp32:XIAO_ESP32S3:PSRAM=opi,FlashMode=qio,FlashSize=8M \
    ESP32_CORE_VERSION=${ESP32_CORE_VERSION}

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        python3 \
        python3-serial \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

ARG TARGETARCH
RUN case "${TARGETARCH}" in \
      amd64) CLI_ARCH=64bit ;; \
      arm64) CLI_ARCH=ARM64 ;; \
      *) echo "unsupported TARGETARCH=${TARGETARCH}" >&2; exit 1 ;; \
    esac \
    && curl -fsSL \
      "https://github.com/arduino/arduino-cli/releases/download/v${ARDUINO_CLI_VERSION}/arduino-cli_${ARDUINO_CLI_VERSION}_Linux_${CLI_ARCH}.tar.gz" \
    | tar -xz -C /usr/local/bin arduino-cli \
    && arduino-cli version

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

WORKDIR /sketch
VOLUME ["/data", "/build", "/sketch"]

ENTRYPOINT ["/entrypoint.sh"]
CMD ["compile"]
