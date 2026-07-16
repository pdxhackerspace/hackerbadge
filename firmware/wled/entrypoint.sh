#!/bin/sh
# Clone (or update) WLED, apply the Hackerbadge PlatformIO override, build firmware.
set -eu

WLED_VERSION="${WLED_VERSION:-v16.0.1}"
PIO_ENV="${PIO_ENV:-hackerbadge}"
WLED_DIR="${WLED_DIR:-/wled}"
BUILD_DIR="${BUILD_DIR:-/build}"
OVERRIDE_SRC="${OVERRIDE_SRC:-/override/platformio_override.ini}"

mkdir -p "$BUILD_DIR" /cache/platformio /cache/npm

if [ ! -f "$WLED_DIR/platformio.ini" ]; then
  echo "Cloning WLED ${WLED_VERSION}..."
  rm -rf "${WLED_DIR:?}/"* "${WLED_DIR}/".[!.]* 2>/dev/null || true
  git clone --depth 1 --branch "$WLED_VERSION" https://github.com/wled/WLED.git "$WLED_DIR"
else
  echo "Using existing WLED tree at $WLED_DIR"
  git -C "$WLED_DIR" fetch --depth 1 origin "refs/tags/${WLED_VERSION}:refs/tags/${WLED_VERSION}" 2>/dev/null \
    || git -C "$WLED_DIR" fetch --depth 1 origin "$WLED_VERSION" || true
  git -C "$WLED_DIR" checkout -f "$WLED_VERSION"
fi

if [ ! -f "$OVERRIDE_SRC" ]; then
  echo "Missing override file: $OVERRIDE_SRC" >&2
  exit 1
fi
cp "$OVERRIDE_SRC" "$WLED_DIR/platformio_override.ini"

cd "$WLED_DIR"

# Prefer WLED's pinned PlatformIO stack when present
if [ -f requirements.txt ]; then
  /opt/pio/bin/pip install --no-cache-dir -r requirements.txt
fi

if [ ! -d node_modules ]; then
  npm ci
else
  npm ci --prefer-offline
fi
npm run build

cmd="${1:-compile}"
if [ "$#" -gt 0 ]; then
  shift
fi

case "$cmd" in
  compile)
    echo "Building WLED env=$PIO_ENV (version=$WLED_VERSION)"
    pio run -e "$PIO_ENV" "$@"
    # Release binaries land in build_output/; also keep the .pio firmware.bin
    if [ -d build_output ]; then
      cp -a build_output/. "$BUILD_DIR/"
    fi
    mkdir -p "$BUILD_DIR/pio"
    if [ -f ".pio/build/${PIO_ENV}/firmware.bin" ]; then
      cp -a ".pio/build/${PIO_ENV}/firmware.bin" "$BUILD_DIR/pio/"
      cp -a ".pio/build/${PIO_ENV}/"*.bin "$BUILD_DIR/pio/" 2>/dev/null || true
    fi
    echo "Build artifacts in $BUILD_DIR"
    ls -la "$BUILD_DIR" "$BUILD_DIR/firmware" "$BUILD_DIR/pio" 2>/dev/null || ls -la "$BUILD_DIR"
    ;;
  upload)
    PORT="${PORT:-${DEVICE:-}}"
    if [ -z "$PORT" ]; then
      echo "Set PORT or DEVICE to the serial port (e.g. /dev/ttyACM0)" >&2
      exit 1
    fi
    echo "Building and uploading WLED env=$PIO_ENV to $PORT"
    pio run -e "$PIO_ENV" -t upload --upload-port "$PORT" "$@"
    ;;
  shell)
    exec /bin/bash "$@"
    ;;
  *)
    exec pio "$cmd" "$@"
    ;;
esac
