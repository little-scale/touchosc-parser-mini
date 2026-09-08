# TouchOSC Parser Mini layout support

The controller can display a TouchOSC Mk2 layout as a full-screen control surface and use the OSC mappings stored in that document.

## Uploading

1. Join the controller to Wi-Fi using its on-device network setup.
2. From a computer on the same network, open `http://<device-name>.local` in a current browser.
3. Choose one `.tosc` file, or select up to eight files together for a multi-page installation.
4. The browser orders multiple files naturally by filename, validates and compiles them locally, then installs the complete page set on the controller.

Choose **Remove installed layout** on the same page to delete the stored layout. The device immediately returns to the TouchOSC Parser Mini home screen; no firmware upload or reboot is required.

The same page shows the OSC target IPv4 address, computer receive port, and device receive port. Saving these values persists them and reconfigures UDP immediately; no reboot or firmware upload is required.

The previous valid layout remains installed if parsing, validation, or upload fails. The layout is stored in the FAT filesystem and survives a normal firmware upload. A persistent Wi-Fi indicator in the top-left opens device settings with a single tap. Its colour shows connectivity. The top-right battery indicator shows remaining charge and adds a lightning symbol while charging.

When more than one page is installed, a small page counter such as `1/4` appears between the Wi-Fi and battery indicators. Tap it to advance to the next page. Each page retains its own canvas size, background, controls, current values and inherited OSC paths. The combined page set may contain up to 64 rendered controls in total.

## Supported document subset

- The root `GROUP` is canvas metadata only.
- Nested `GROUP` controls are rejected.
- Interactive controls: `BUTTON`, `FADER`, `RADIAL`, `XY`, `RADAR`, and `ENCODER`.
- `GRID` is supported with its materialized child controls; child frames are resolved relative to the grid.
- `LABEL` is rendered as a single line and `TEXT` as multiple lines, including font size, alignment, text colour, background, outline, clipping, and text wrapping.
- Up to eight pages and 64 rendered controls across the complete page set.
- Canvas background, control color and alpha, frames, visibility, interactivity, shape, background, outline style, grids, bars, cursors, lines, inversion, centering, axis locks, and absolute or relative response.
- One or two OSC float arguments sourced from `x` and `y`, or one OSC string argument sourced from `text`.
- OSC path partials using constants, `name`, `parent.name`, and child index. TouchOSC `INDEX` uses the raw zero-based position in the parent's child list before applying `out = min + index × (max − min)` and the requested conversion; this preserves distinct addresses for grid children. A persistent setting chooses whether outgoing layout messages use the document path directly or prefix it with `/<device-name>` so messages from multiple boards remain distinguishable. Incoming prefixed feedback is stripped back to the document path before matching, and unprefixed feedback is also accepted.
- Send and receive flags. Incoming values obey local-touch ownership and are never echoed.

The common TouchOSC `orientation` property is retained as `NORTH`, `EAST`, `SOUTH`, or `WEST`. Faders render and respond along the corresponding axis and direction. XY coordinates and grids rotate with the control, while radial, encoder, and radar controls rotate their angular origin. Buttons currently use momentary values only, so orientation has no visible effect unless positional button values are added in the future.

Layouts containing unsupported controls or OSC expressions are rejected with an explanation in the upload page. MIDI, scripts, radio controls, pagers, arbitrary groups, mixed OSC argument types, multiple OSC messages per control, and more than two numeric OSC arguments are not part of the runtime subset.

## Rendering notes

Layouts designed at 368 × 448 use native full-screen coordinates. A document no taller than 400 pixels is treated as a status-safe layout: it is proportionally fitted within the 368 × 400 area and anchored to the bottom of the display, leaving the top 48-pixel settings/status row unobstructed. Taller documents retain full-screen proportional scaling and centered letterboxing. Colors are composited against the document background and converted to RGB565 for the panel. TouchOSC-style inactive fills, active fills, grid lines, bracket outlines, rotary arcs, radar amplitude rings, and control cursors are reproduced with the display's available raster primitives. Fader grid markings are overlaid after the bar and cursor so the scale remains visible in both filled and unfilled regions.

During interaction, the runtime redraws and transfers only the changed control's aligned display region. Initial layout loads, settings transitions, and simultaneous updates to multiple controls still use a full-screen refresh. Touch is sampled every 8 ms, display updates are capped at 50 Hz, and OSC value output is capped at 50 Hz independently of display transfer time.

Because the AMOLED panel, RGB565 color depth, and embedded rasterizer differ from a desktop display, exact emitted light and anti-aliasing cannot be identical. Native geometry, state, layering, and OSC behavior are the compatibility targets.

The embedded font is a compact bitmap approximation of TouchOSC's default and monospaced typefaces. Label and text geometry, requested size, alignment, line breaks, clipping, wrapping, and colour are retained. Incoming OSC string messages update matching `LABEL` and `TEXT` controls without echoing them.
