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
- Persistent on-device toggle for 25 Hz IMU output over OSC
- Large-key on-device keyboards for Wi-Fi, device-name and OSC entry, with a portable [keyboard implementation guide](LARGE_KEYBOARD_PORTING.md)
- Ready-to-upload screen shows both the device's `.local` hostname and numeric IP address
- Optional private provisioning file for flashing the same Wi-Fi and OSC destination to multiple boards, with a portable [Micro Apps porting guide](PROVISIONING_PORTING.md)
- Optional git-ignored default layout bundle for rapid private multi-board testing
- OSC send and receive over UDP with local-touch ownership and no echo
- Responsive partial redraws capped at 50 frames per second
- TouchOSC document paths retained internally, with a persistent option to send them directly (the default) or under a unique `/<device-name>` OSC prefix
- North, east, south and west orientation for faders and other directional controls

Supported controls are `BUTTON`, `FADER`, `RADIAL`, `XY`, `RADAR`, `ENCODER`, `GRID` children, `LABEL`, and `TEXT`. The document root `GROUP` supplies canvas metadata; nested groups are intentionally unsupported.

## Using it

1. Tap the settings cog in the top-left of the device and connect it to a network. Its colour shows Wi-Fi status: green when connected and grey when offline.
2. On a computer on the same network, open `http://<device-name>.local`.
3. Select one TouchOSC Mk2 `.tosc` file, or select several files together to install them as pages. Files are ordered naturally by filename.
4. Enter the computer's IPv4 address and OSC ports in the browser page or on the device.
5. Use **SEND NAME ON/OFF** in device settings, or the matching browser option, to choose between `/device-8428/fader1` and the original `/fader1` style of layout address.
6. Tap the three-axis symbol beside the cog to enable or disable IMU streaming. Pink means enabled and grey means disabled.
7. For a multi-page set, tap the page counter between the status controls and battery icon to advance pages.
8. Use **Remove installed layout** to delete the stored layout and return the device to its home screen.

The default OSC setup sends to UDP port `9000` and listens on UDP port `9001`.

## Provisioning multiple boards

To build one private firmware image that gives multiple boards the same Wi-Fi and OSC destination, copy the example file:

```sh
cp firmware/Provisioning.local.h.example firmware/Provisioning.local.h
```

Edit `firmware/Provisioning.local.h` with the shared network name, password, target IP address, and ports, then run the normal build. The local file is excluded from Git. Keep the resulting firmware binary private too, because its Wi-Fi password can be extracted by someone who obtains it.

Every board still generates its own `device-xxxx` name from its hardware identity. This keeps device-owned OSC addresses such as IMU streams distinct even though all boards send to the same computer and UDP port.

Provisioning uses a revision number:

- A blank board accepts revision `1` on first boot.
- Reflashing the same revision preserves settings changed through the device or browser.
- Increasing the revision deliberately reapplies the shared Wi-Fi and OSC values on the next boot.

Do not increase the revision for an ordinary firmware update.

### Private default layouts

A private build can also install an initial multi-page layout set. Compile the desired `.tosc` files into the git-ignored header before building:

```sh
python3 scripts/compile_touchosc.py \
  --header firmware/DefaultLayout.local.h \
  --revision 1 \
  /path/to/page1.tosc /path/to/page2.tosc
```

Input files are naturally ordered by filename. Neither the `.tosc` sources nor `DefaultLayout.local.h` belongs in Git or a public release. The generated compact bundle is validated by the device before installation.

Layout revisions behave like credential revisions: the same revision does not overwrite later browser uploads or reinstall a layout the user removed. Increasing the private layout revision deliberately installs the new defaults once on each board.

## IMU OSC output

IMU streaming is disabled by default and its state persists across restarts. When enabled, the device sends one OSC message at 25 Hz:

| Address | Arguments |
| --- | --- |
| `/<device-name>/imu0` | nine `float32` values: `ax ay az gx gy gz pitch roll yaw` |

Acceleration is measured in g, angular velocity in degrees per second, and orientation in degrees. Pitch and roll are accelerometer-corrected; yaw is relative and will drift because the QMI8658 is a six-axis sensor without a magnetometer.

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

This firmware began from portions of [Micro Apps for ESP32-S3 Touch AMOLED 1.8](https://github.com/little-scale/micro-apps-for-esp32-s3-touch-amoled-1.8) by Sebastian Tomczak. As the copyright holder of that original code, Sebastian Tomczak has relicensed the first-party code used by TouchOSC Parser Mini under the [MIT License](LICENSE).

Bundled third-party libraries are not covered by the project's MIT licence. They retain their own licences and notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
