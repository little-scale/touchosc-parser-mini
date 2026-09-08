# TouchOSC Parser Mini developer handover

**Last updated:** 9 September 2026
**Target:** Waveshare ESP32-S3-Touch-AMOLED-1.8 V2  
**Display:** 368 × 448 CO5300 AMOLED  
**Touch:** CST820 single-point capacitive touch  
**Build:** 1,208,327 bytes program storage; 114,508 bytes static RAM (private five-page bundle revision 4)

TouchOSC Parser Mini is a standalone embedded layout player. It displays a supported subset of TouchOSC Mk2 layouts, sends locally generated values over OSC using the paths in those documents, and accepts matching OSC feedback without echo.

TouchOSC is fantastic software. TouchOSC Parser Mini is an independent community project and is not affiliated with, endorsed by, sponsored by, or associated with TouchOSC or its developers. TouchOSC is a trademark of its respective owner.

First-party TouchOSC Parser Mini code is licensed under the MIT License, copyright 2026 Sebastian Tomczak. Code under `vendor/` retains its own third-party licences and notices.

## Current behavior

- One persistent installed layout set in FFat.
- Browser upload of one `.tosc` file or up to eight files selected together.
- Multiple files are naturally ordered by filename and become pages.
- A centered top badge shows the current page and advances it when tapped.
- The top-left settings cog opens on-device settings; green means Wi-Fi connected and grey means offline.
- The adjacent three-axis badge toggles persistent 25 Hz IMU OSC output; pink means enabled and grey means disabled.
- System UI text never uses the 1x bitmap font; the minimum is the same 2x size used for `READY TO UPLOAD`.
- Wi-Fi and device-name entry use the large-key paged keyboard documented in `LARGE_KEYBOARD_PORTING.md`.
- The ready-to-upload screen shows both the `.local` hostname and numeric IPv4 address and redraws if either connection state or address changes.
- The top-right badge shows battery charge and charging state.
- Removing the layout in the browser returns immediately to the Parser Mini home screen.
- With no layout, no inherited micro-app pages appear.
- OSC destination and receive ports can be edited in the browser or on the device.
- OSC paths come from the layout. A persistent `SEND NAME` setting selects either the raw layout path (the default) or `/<device-name><layout-path>` for outgoing layout messages; the prefix is removed before incoming layout matching.
- Incoming values never echo and cannot take ownership from a locally touched control.
- Optional local provisioning seeds shared Wi-Fi and OSC values while retaining a unique MAC-derived device name on each board.
- The provisioning design is documented for reuse in Micro Apps in `PROVISIONING_PORTING.md`.
- An optional git-ignored default layout bundle can install private test pages once per layout revision.
- Control flag bits 22–23 retain TouchOSC orientation (`NORTH=0`, `EAST=1`, `SOUTH=2`, `WEST=3`) without changing the format-v3 record size.

The current multi-page firmware has compiled, passed embedded JavaScript syntax checking, and been flashed. The private five-page bundle loads successfully; control interaction and OSC behaviour still need physical acceptance testing after each relevant change.

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

`TouchOscLayout` owns parsed page/control state and event generation. Layouts are proportionally scaled and horizontally centered, with vertical placement determined by canvas height. The full-screen PSRAM canvas is authoritative, but interaction redraws normally transfer only the changed control's even-aligned rectangle to the CO5300. Full redraws occur on layout load, page change, settings transitions, and simultaneous control updates.

Canvas pages no taller than 400 pixels opt into the status-safe area. They are fitted within 368 × 400 and bottom-aligned, placing a native 368 × 400 document at `y=48…447`. Pages taller than 400 pixels keep the legacy full-screen centered transform. Rendering, hit testing, and partial redraws all use the same transform.

Touch is sampled every 8 ms. Display and OSC streaming are capped independently at 50 Hz, with a final OSC value sent on release. The centered page badge is reserved at `x 132…235, y 0…47`; tapping it advances pages. The settings cog uses the corresponding top-left reserved region.

Settings buttons act on touch-down. `closeWifiSetup()` ends any layout gesture and ignores touch until finger-up, with a 500 ms safety timeout. Preserve this transition guard: without it, the EXIT finger can leak into the restored layout and make touch appear frozen.

## OSC rules

- The device-owned IMU endpoint is `/<device-name>/imu0` with nine floats: acceleration XYZ, gyroscope XYZ, then pitch, roll and relative yaw.
- IMU streaming is disabled by default, runs at 25 Hz when enabled, and persists through `imu_out` in Preferences.
- One or two float arguments mapped from `x` and `y`, or one string argument mapped from `text`.
- Paths may use constants, `name`, `parent.name`, and grid-child index partials. `INDEX` is the raw zero-based child position and is scaled as `min + index × (max − min)` before conversion; do not normalize it by child count. Outgoing layout messages add the device-name prefix only when `layout_prefix` is enabled in Preferences. Incoming prefixed feedback removes it before lookup, while unprefixed feedback remains accepted in either mode.
- Send and receive flags are retained.
- Local interaction sends OSC; remote changes redraw only.
- Remote changes to the actively touched control are ignored until release.
- Values are normalized to `0.0…1.0`.

## Repository map

| Path | Purpose |
| --- | --- |
| `firmware/firmware.ino` | Composition root, Wi-Fi, mDNS, OSC and event routing |
| `firmware/ImuService.*` | Minimal QMI8658 sampling, screen-axis mapping and orientation filter |
| `firmware/TouchOscLayout.*` | Compact format parser, state, rendering and touch interaction |
| `firmware/LayoutServer.*` | Browser UI, upload/removal endpoints and OSC settings |
| `firmware/UserInterface.*` | Display/touch hardware, status overlays, landing and settings screens |
| `firmware/OscTransport.*` | OSC 1.0 UDP encoding and decoding |
| `firmware/ConfigStore.*` | Persistent settings |
| `firmware/Provisioning.h` | Private provisioning loader and safe checked-in defaults |
| `firmware/Provisioning.local.h.example` | Template for the git-ignored fleet settings file |
| `firmware/DefaultLayout.h` | Inert public loader for an optional private compiled layout bundle |
| `vendor/waveshare-v2` | Pinned V2 hardware libraries |
| `scripts/build.sh` | Reproducible Arduino CLI build |
| `scripts/compile_touchosc.py` | Host-side `.tosc` compiler and private header generator |
| `scripts/flash.sh` | Standard explicit-port flashing helper |

The unused `AudioService`, `BleTransport`, and `ConfigPortal` files have been removed. `ImuService` is active and supplies the optional OSC stream. Legacy micro-app members/functions remain temporarily inside `UserInterface`; they are unreachable from the standalone composition root and should be removed carefully after the multi-page acceptance test.

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

### Private fleet provisioning

`firmware/Provisioning.local.h` is git-ignored. When present, its shared Wi-Fi and OSC values are compiled into the application. `TPM_PROVISIONING_REVISION` controls application: a revision newer than the `prov_rev` value in Preferences is applied once and then recorded. Rebuilding with the same revision does not overwrite subsequent on-device edits. Increasing the revision is an explicit fleet reprovisioning action.

The private file deliberately has no device-name field. `firmware.ino` derives `device-xxxx` from the final two displayed bytes of each ESP32-S3 MAC, so multiple boards sharing a destination remain distinguishable. It also migrates names produced by the earlier vendor-prefix bug. Never print provisioning secrets or add the local header or a provisioned binary to GitHub.

### Private default layout bundle

`scripts/compile_touchosc.py` mirrors the browser compiler and naturally orders one to eight input filenames. With `--header firmware/DefaultLayout.local.h --revision N`, it emits format-v3 bytes into a git-ignored header. On boot, `DefaultLayout.h` exposes inert values for public builds or the private bytes when present. A newer revision than Preferences key `layout_rev` is written through `TouchOscLayout::installEmbedded()`, validated using the normal parser, atomically activated, and then recorded.

The revision is independent of credential provisioning. Reusing it preserves browser-uploaded or deliberately removed layouts; incrementing it explicitly reapplies the embedded set. Never commit the generated header, source layouts, or a binary containing private defaults.

## Next work

1. Exercise every private page, including the horizontal page-2 fader, and verify namespaced OSC output and feedback.
2. Exercise page switching, settings exit and layout removal.
3. Remove the unreachable micro-app UI implementation.
4. Reduce `AppTypes.h`, `UserInterface.h`, and `OscTransport` to Parser Mini concepts.
5. Add a small host-side fixture test for compact format versions 1–3.
