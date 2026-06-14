#!/bin/zsh
set -euo pipefail

PROJECT_DIR="${1:-$(pwd)}"
ENV_NAME="${2:-lilygo-t-display-s3}"
VERSION="${3:-v0.1.0}"
OUT_DIR="$PROJECT_DIR/release"
BUILD_DIR="$PROJECT_DIR/.pio/build/$ENV_NAME"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

mkdir -p "$OUT_DIR"
cp "$BUILD_DIR/bootloader.bin" "$OUT_DIR/"
cp "$BUILD_DIR/partitions.bin" "$OUT_DIR/"
cp "$BUILD_DIR/firmware.bin" "$OUT_DIR/"
if [[ -f "$BUILD_DIR/firmware.factory.bin" ]]; then
  cp "$BUILD_DIR/firmware.factory.bin" "$OUT_DIR/"
fi
cp "$BOOT_APP0" "$OUT_DIR/"

cat > "$OUT_DIR/manifest.json" <<MANIFEST
{
  "name": "WLEDRemote+",
  "version": "$VERSION",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "bootloader.bin", "offset": 4096 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "firmware.bin", "offset": 65536 }
      ]
    }
  ]
}
MANIFEST

(
  cd "$OUT_DIR"
  shasum -a 256 bootloader.bin partitions.bin boot_app0.bin firmware.bin firmware.factory.bin 2>/dev/null > sha256sums.txt || \
  shasum -a 256 bootloader.bin partitions.bin boot_app0.bin firmware.bin > sha256sums.txt
)

echo "Release assets prepared in: $OUT_DIR"
