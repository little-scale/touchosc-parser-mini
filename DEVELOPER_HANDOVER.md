# TouchOSC Parser Mini developer handover

**Last updated:** 8 September 2026  
**Target:** Waveshare ESP32-S3-Touch-AMOLED-1.8 V2  
**Display:** 368 × 448 CO5300 AMOLED  
**Touch:** CST820 single-point capacitive touch  
**Build:** 1,200,059 bytes program storage; 114,428 bytes static RAM

TouchOSC Parser Mini is a standalone embedded layout player. It displays a supported subset of TouchOSC Mk2 layouts, sends locally generated values over OSC using the paths in those documents, and accepts matching OSC feedback without echo.

TouchOSC is fantastic software. TouchOSC Parser Mini is an independent community project and is not affiliated with, endorsed by, sponsored by, or associated with TouchOSC or its developers. TouchOSC is a trademark of its respective owner.

First-party TouchOSC Parser Mini code is licensed under the MIT License, copyright 2026 Sebastian Tomczak. Code under `vendor/` retains its own third-party licences and notices.

## Current behavior

- One persistent installed layout set in FFat.
- Browser upload of one `.tosc` file or up to eight files selected together.
- Multiple files are naturally ordered by filename and become pages.
- A centered top badge shows the current page and advances it when tapped.
- The top-left Wi-Fi badge opens on-device settings.
- The top-right badge shows battery charge and charging state.
- Removing the layout in the browser returns immediately to the Parser Mini home screen.
- With no layout, no inherited micro-app pages appear.
- OSC destination and receive ports can be edited in the browser or on the device.
- OSC paths come from the layout and are not placed under the device hostname.
- Incoming values never echo and cannot take ownership from a locally touched control.

The current multi-page firmware has compiled, passed embedded JavaScript syntax checking, and been flashed. Uploading and switching the four example pages still needs physical acceptance testing.

## Supported layout subset

Supported control types:

- `BUTTON`
- `FADER`
- `RADIAL`
- `XY`
- `RADAR`
- `ENCODER`
- `GRID` with materialized child controls
- `LABEL`
- `TEXT`

The document root `GROUP` supplies canvas metadata. Nested groups, pagers, scripts, MIDI-only mappings, and controls outside the supported list are rejected. Each page may have its own canvas dimensions and background. The complete installed set is limited to eight pages, 64 rendered controls, and a 32 KB compact upload.

## Browser compilation

`LayoutServer.cpp` embeds the management page. The browser uses `DecompressionStream('deflate')` to unpack each `.tosc` file, parses its `lexml` document with `DOMParser`, validates the supported subset, and emits a compact binary representation. The original documents are not stored on the ESP32.

For multi-file upload, each document is first compiled as a version-2 page record. The browser then combines those records into format version 3:

```text
TLAY
u8  version = 3
u8  total control count
u16 first page width
u16 first page height
rgba first page background
u16 page count
page headers 2..N: u16 width, u16 height, rgba background
control records: u8 type, u8 page index, remaining version-2 fields
```

The device continues to read existing version-1 and version-2 single-page files. Version 3 supports up to eight pages. The browser sorts selected filenames using natural numeric ordering, so `page2.tosc` precedes `page10.tosc`.

## Rendering and interaction

`TouchOscLayout` owns parsed page/control state and event generation. Layouts are proportionally scaled and centered. The full-screen PSRAM canvas is authoritative, but interaction redraws normally transfer only the changed control's even-aligned rectangle to the CO5300. Full redraws occur on layout load, page change, settings transitions, and simultaneous control updates.

Touch is sampled every 8 ms. Display and OSC streaming are capped independently at 50 Hz, with a final OSC value sent on release. The centered page badge is reserved at `x 132…235, y 0…47`; tapping it advances pages. Wi-Fi uses the corresponding top-left reserved region.

Settings buttons act on touch-down. `closeWifiSetup()` ends any layout gesture and ignores touch until finger-up, with a 500 ms safety timeout. Preserve this transition guard: without it, the EXIT finger can leak into the restored layout and make touch appear frozen.

## OSC rules

- One or two float arguments mapped from `x` and `y`, or one string argument mapped from `text`.
- Paths may use constants, `name`, `parent.name`, and grid-child index partials.
- Send and receive flags are retained.
- Local interaction sends OSC; remote changes redraw only.
- Remote changes to the actively touched control are ignored until release.
- Values are normalized to `0.0…1.0`.

## Repository map

| Path | Purpose |
| --- | --- |
| `firmware/firmware.ino` | Composition root, Wi-Fi, mDNS, OSC and event routing |
| `firmware/TouchOscLayout.*` | Compact format parser, state, rendering and touch interaction |
| `firmware/LayoutServer.*` | Browser UI, upload/removal endpoints and OSC settings |
| `firmware/UserInterface.*` | Display/touch hardware, status overlays, landing and settings screens |
| `firmware/OscTransport.*` | OSC 1.0 UDP encoding and decoding |
| `firmware/ConfigStore.*` | Persistent settings |
| `vendor/waveshare-v2` | Pinned V2 hardware libraries |
| `scripts/build.sh` | Reproducible Arduino CLI build |
| `scripts/flash.sh` | Standard explicit-port flashing helper |

The unused `AudioService`, `BleTransport`, `ImuService`, and `ConfigPortal` files have been removed. Legacy micro-app members/functions remain temporarily inside `UserInterface`; they are unreachable from the standalone composition root and should be removed carefully after the multi-page acceptance test.

## Build and flash

The build uses Espressif Arduino core 3.3.10 with:

```text
FlashSize=16M
PartitionScheme=app3M_fat9M_16MB
PSRAM=opi
USBMode=hwcdc
CDCOnBoot=cdc
```

Build from the repository root:

```sh
./scripts/build.sh
```

This specific board has not reliably entered download mode through the standard reset sequence. The verified recovery flow is:

1. Hold BOOT.
2. Connect with esptool using `--before usb-reset --after no-reset`.
3. Release BOOT after the flasher stub is running.
4. Write only `build/output/firmware.ino.bin` at `0x10000` using `--before no-reset --after watchdog-reset`.

Writing only the application partition preserves FFat layouts and Preferences. Do not write the full merged 16 MB image when preservation matters.

## Next work

1. Upload the four sample pages together and verify the expected 4 pages / 13 controls result.
2. Exercise every page, page switching, OSC output, OSC feedback, settings exit and layout removal.
3. Remove the unreachable micro-app UI implementation.
4. Reduce `AppTypes.h`, `UserInterface.h`, and `OscTransport` to Parser Mini concepts.
5. Add a small host-side fixture test for compact format versions 1–3.
