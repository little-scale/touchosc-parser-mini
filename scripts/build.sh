#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc'

mkdir -p "$PROJECT_ROOT/build/cache" "$PROJECT_ROOT/build/output"
arduino-cli compile \
  --fqbn "$FQBN" \
  --libraries "$PROJECT_ROOT/vendor/waveshare-v2" \
  --build-path "$PROJECT_ROOT/build/cache" \
  --output-dir "$PROJECT_ROOT/build/output" \
  "$PROJECT_ROOT/firmware"
