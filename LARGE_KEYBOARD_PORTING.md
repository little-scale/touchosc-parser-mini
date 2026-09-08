# Large on-device keyboard porting guide

This document describes the large-key keyboard used by TouchOSC Parser Mini on the Waveshare ESP32-S3-Touch-AMOLED-1.8 V2. It is intended to make the same keyboard easy to reproduce in the Micro Apps firmware without bringing across unrelated Parser Mini code.

The implementation is first-party TouchOSC Parser Mini code and is available under the repository's MIT License.

## Design target

- Display: 368 × 448 pixels
- Touch: CST820 single-point capacitive touch
- Text: Arduino GFX built-in bitmap font at `setTextSize(2)`
- Input behavior: one key action on the initial touch-down; holding a finger does not repeat
- Main goal: keep every frequently used key close to 47 × 46 pixels while still providing all printable characters normally needed in a Wi-Fi password

## Shared key grid

The character area starts at `y = 110` and contains four rows on a 50-pixel pitch.

```cpp
constexpr int16_t screenWidth = 368;
constexpr int16_t leftMargin = 8;
constexpr int16_t keyGap = 3;
constexpr int16_t firstRowY = 110;
constexpr int16_t rowPitch = 50;
constexpr int16_t keyDrawHeight = 46;
constexpr int16_t keyTouchHeight = 48;

const uint8_t count = strlen(keys);
const int16_t keyWidth =
    (screenWidth - 2 * leftMargin - (count - 1) * keyGap) / count;
const int16_t keyX = leftMargin + column * (keyWidth + keyGap);
const int16_t keyY = firstRowY + row * rowPitch;
```

Draw each key as a rounded rectangle at `(keyX, keyY)` with size `keyWidth × 46`, radius 4. Centre its one-character label at `keyY + 15` using text size 2.

For touch detection, deliberately include the 3-pixel visual gap on the right of each key. The active rectangle is `keyWidth + keyGap` by 48 pixels. This makes the keyboard more forgiving without visibly crowding it. Ignore an empty row rather than calculating a width for it.

## Wi-Fi password keyboard

Keep these two state values:

```cpp
uint8_t keyboardMode = 0;  // 0 letters, 1 common, 2 more
bool shift = false;
```

Use these exact pages:

```cpp
static const char *lower[]  = {"abcdefg", "hijklmn", "opqrstu", "vwxyz"};
static const char *upper[]  = {"ABCDEFG", "HIJKLMN", "OPQRSTU", "VWXYZ"};
static const char *common[] = {"1234567", "890-_.@", "!#$%&*+", "=/?():;"};
static const char *more[]   = {"\"',<>", "[]{}", "\\^`|~", ""};
```

`keyboardMode == 0` selects `lower` or `upper` according to `shift`. Mode 1 selects `common`; mode 2 selects `more`.

The control row is at `y = 314`, height 46:

| Position | Size | Letters mode | Common mode | More mode |
| --- | --- | --- | --- | --- |
| `x=8` | `80×46` | `ABC` or `abc` | `-` | `-` |
| `x=94` | `80×46` | `123` | `MORE` | `ABC` |
| `x=180` | `98×46` | `SPACE` | `SPACE` | `SPACE` |
| `x=284` | `76×46` | `DEL` | `DEL` | `DEL` |

The first button toggles shift only while on the letters page. The second advances the page using `(keyboardMode + 1) % 3`. `SPACE` appends one space and `DEL` removes the last character.

Reset the keyboard to lower-case letters whenever a network is selected:

```cpp
shift = false;
keyboardMode = 0;
```

Parser Mini limits the password to 63 characters. Its password field shows only the newest 20 characters, prefixes the display with `>` when older text is hidden, and supports a separate `SHOW`/`HIDE` button.

## Device-name keyboard

The device-name screen uses the same geometry with a simpler two-page character set:

```cpp
static const char *letters[] = {"abcdefg", "hijklmn", "opqrstu", "vwxyz"};
static const char *numbers[] = {"1234567", "890-", "", ""};
bool numbersPage = false;
```

Its control row is:

| Position | Size | Action |
| --- | --- | --- |
| `x=8` | `80×46` | Toggle `123` / `ABC` |
| `x=94` | `80×46` | `CLEAR` |
| `x=180` | `98×46` | `DEFAULT` |
| `x=284` | `76×46` | `DEL` |

Reset `numbersPage` to false when entering the device-name screen. Parser Mini limits names to 24 characters.

## OSC numeric keypad

The OSC settings screen uses a separate 3 × 4 keypad for IPv4 addresses and ports:

```cpp
static const char keys[] = "123456789.0<";
static const int16_t keyX[3] = {10, 130, 250};

const int16_t x = keyX[column];
const int16_t y = 178 + row * 47;
```

Draw each key at `108 × 45`. Use `DEL` as the visible label for `<`. For touch detection, expand each key left by 5 pixels and use a `118 × 47` target. Disable the decimal point for the two port fields.

## Full-screen placement

The password and device-name screens use these vertical regions:

| Region | Y range |
| --- | --- |
| Title | around `y=12` |
| Text field | `y=48`, height 48 |
| Four character rows | `y=110…305` |
| Keyboard controls | `y=314`, height 46 |
| Back / Connect or Save | `y=378`, height 56 |

The bottom buttons are `BACK` at `x=10`, width 168, and `CONNECT` or `SAVE` at `x=190`, width 168.

## Integration checklist

1. Handle keys on touch-down, not touch release, and suppress additional actions until the finger is lifted.
2. Redraw the screen after every key, page, shift, delete, clear, default, or visibility action.
3. Keep the visual gap between keys, but include it in the preceding key's touch target.
4. Reset keyboard mode when entering an input screen so it always opens predictably.
5. Enforce the destination field's character limit before appending.
6. Preserve a large `BACK` and primary action button below the keyboard.
7. Use text size 2 as the minimum for all system-interface text on this display.

The reference implementation is in `firmware/UserInterface.cpp`, principally `keyboardRow()`, `deviceKeyboardRow()`, `drawWifiKeyboard()`, `drawDeviceSettings()`, `drawOscSettings()`, and `handleWifiSetupTap()`.
