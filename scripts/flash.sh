#!/bin/sh
# SPDX-FileCopyrightText: 2026 Sebastian Tomczak
# SPDX-License-Identifier: MIT

set -eu

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 /dev/cu.usbmodem…" >&2
  exit 2
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc'

if [ ! -f "$PROJECT_ROOT/build/output/firmware.ino.bin" ]; then
  echo "No compiled firmware found. Run ./scripts/build.sh first." >&2
  exit 1
fi

arduino-cli upload \
  --fqbn "$FQBN" \
  --port "$1" \
  --input-dir "$PROJECT_ROOT/build/output" \
  --verify
