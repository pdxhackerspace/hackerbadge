#!/bin/sh
# Install board/core + libraries (cached under /data), then compile or upload.
set -eu

SKETCH_DIR="${SKETCH_DIR:-/sketch/badge_test}"
BUILD_DIR="${BUILD_DIR:-/build}"
FQBN="${FQBN:-esp32:esp32:XIAO_ESP32S3:PSRAM=opi,FlashMode=qio,FlashSize=8M}"
ESP32_CORE_VERSION="${ESP32_CORE_VERSION:-3.1.1}"
ADDITIONAL_URL="https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"

mkdir -p "$ARDUINO_DIRECTORIES_DATA" "$ARDUINO_DIRECTORIES_USER" "$BUILD_DIR"

if [ ! -f "$ARDUINO_DIRECTORIES_DATA/arduino-cli.yaml" ]; then
  arduino-cli config init
fi

arduino-cli config set board_manager.additional_urls "$ADDITIONAL_URL"
arduino-cli core update-index
arduino-cli core install "esp32:esp32@${ESP32_CORE_VERSION}"

# Libraries needed by badge_test.ino (Adafruit LSM6DS pulls BusIO + Unified Sensor)
arduino-cli lib install \
  FastLED \
  "Adafruit LSM6DS" \
  "Adafruit BusIO" \
  "Adafruit Unified Sensor"

cmd="${1:-compile}"
if [ "$#" -gt 0 ]; then
  shift
fi

case "$cmd" in
  compile)
    echo "Compiling $SKETCH_DIR with FQBN=$FQBN"
    arduino-cli compile \
      --fqbn "$FQBN" \
      --build-path "$BUILD_DIR/build" \
      --output-dir "$BUILD_DIR" \
      "$SKETCH_DIR" \
      "$@"
    echo "Build artifacts in $BUILD_DIR"
    ;;
  upload)
    PORT="${PORT:-${DEVICE:-}}"
    if [ -z "$PORT" ]; then
      echo "Set PORT or DEVICE to the serial port (e.g. /dev/ttyACM0)" >&2
      exit 1
    fi
    echo "Uploading $SKETCH_DIR to $PORT with FQBN=$FQBN"
    arduino-cli compile \
      --fqbn "$FQBN" \
      --build-path "$BUILD_DIR/build" \
      --output-dir "$BUILD_DIR" \
      --upload \
      --port "$PORT" \
      "$SKETCH_DIR" \
      "$@"
    ;;
  shell)
    exec /bin/sh "$@"
    ;;
  *)
    exec arduino-cli "$cmd" "$@"
    ;;
esac
