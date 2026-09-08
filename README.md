# TouchOSC Parser Mini

TouchOSC Parser Mini is a small, standalone TouchOSC layout parser and OSC control runtime for the **Waveshare ESP32-S3-Touch-AMOLED-1.8 V2** (368 × 448, CO5300 display and CST820 touch controller).

Upload a TouchOSC Mk2 `.tosc` document from a browser and the ESP32-S3 displays it as an interactive control surface. Touch values are sent to a configured host using the OSC paths inherited from the layout document, and matching incoming OSC messages update the display without being echoed.

> **Independent project notice:** TouchOSC is fantastic software. TouchOSC Parser Mini is an independent community project and is not affiliated with, endorsed by, sponsored by, or associated with TouchOSC or its developers. TouchOSC is a trademark of its respective owner.

## Current features

- Browser-based single- or multi-file `.tosc` upload, local parsing and device installation
- One persistent layout slot that survives an ordinary firmware update
- Explicit **Remove installed layout** action
- A dedicated Parser Mini home screen when no layout is installed
- On-device Wi-Fi, OSC target and device-name settings
- Persistent Wi-Fi and battery/charging indicators
- OSC send and receive over UDP with local-touch ownership and no echo
- Responsive partial redraws capped at 50 frames per second
- TouchOSC document paths retained rather than rewritten under a device prefix

Supported controls are `BUTTON`, `FADER`, `RADIAL`, `XY`, `RADAR`, `ENCODER`, `GRID` children, `LABEL`, and `TEXT`. The document root `GROUP` supplies canvas metadata; nested groups are intentionally unsupported.

## Using it

1. Tap the Wi-Fi icon in the top-left of the device and connect it to a network.
2. On a computer on the same network, open `http://<device-name>.local`.
3. Select one TouchOSC Mk2 `.tosc` file, or select several files together to install them as pages. Files are ordered naturally by filename.
4. Enter the computer's IPv4 address and OSC ports in the browser page or on the device.
5. For a multi-page set, tap the page counter between the Wi-Fi and battery icons to advance pages.
6. Use **Remove installed layout** to delete the stored layout and return the device to its home screen.

The default OSC setup sends to UDP port `9000` and listens on UDP port `9001`.

See [TOUCHOSC_LAYOUTS.md](TOUCHOSC_LAYOUTS.md) for the supported document subset and rendering details.

## Build

The project uses Arduino CLI with Espressif Arduino core 3.3.10. Board support libraries for the V2 hardware are pinned under `vendor/waveshare-v2`.

```sh
./scripts/build.sh
```

To flash an explicitly detected serial port:

```sh
./scripts/flash.sh /dev/cu.usbmodemXXXX
```

Do not use a V1 board configuration: that revision uses different display and touch hardware.

## Origins and licence

This firmware began from portions of [Micro Apps for ESP32-S3 Touch AMOLED 1.8](https://github.com/little-scale/micro-apps-for-esp32-s3-touch-amoled-1.8) by Sebastian Tomczak. TouchOSC Parser Mini remains available under the GNU General Public License version 3 in accordance with that project's licence. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for attribution and the licences included with the pinned vendor libraries.
