# GhostESP port: ESP32-2424S012

Buildable bring-up port for the **ESP32-C3, 4 MB flash, GC9A01 240×240 round
LCD and CST816D/S capacitive touch** board. This is the bare board, with no
GPS, microSD or PSRAM.

This source-only copy omits the installed toolchain, upstream checkout, build
tree, historical device backups and packaged binaries. A build fetches the
pinned dependencies and creates `.tools/`, `upstream/`, `build/` and `dist/`.

**Status:** cross-compilation, resolved configuration checks, flash partition
checks and host touch-frame tests pass. Flashed to the attached ESP32-C3 on
15 November 2026; esptool verified the written flash hash. A 22-second serial
boot check reached the command console and started the GhostNet access point
without a reset loop. The LCD renders and physical touch has reached menus;
later AP and BLE scans passed on the board, while sustained touch operation
remains unverified. The first
profile reported **12,780 bytes of free internal heap (7.2%)** after initialization
and only **4,692 bytes** during USB/menu diagnostics. A freeze was reported on
returning from BadBLE. Heap integrity passed after reconnecting, but the exact
stall was not captured. This revision reserves six Wi-Fi static TX buffers
instead of sixteen (about 16 KB less), and lists only the built-in BadBLE script
on this SD-less board. After flashing, the same USB heap diagnostic reports
**22,188 bytes free**, a **12,288-byte largest block**, and valid heap integrity.
USB screenshots respond. This is an improvement, not a confirmed root-cause
diagnosis or proof of sustained operation. Fewer TX buffers can limit burst
throughput; Wi-Fi performance has not been measured.
After further runtime/web activity, free internal RAM was 20,848 bytes but its
recorded minimum was only 628 bytes (DMA minimum 60 bytes). Heap integrity still
passed. Transient memory exhaustion remains a concern; this is not a stability
sign-off for the full upstream feature set.

The development checkout retained the original 4 MB flash backup and startup
log; those records are not included in this source copy. Boot warnings included
unsupported power-management configuration, an I2C bus-not-initialized message,
and the upstream input task falling back from PSRAM to internal RAM. Touch
operation cannot be established by the serial boot check alone.

## Source and toolchain

- [GhostESP Revival](https://github.com/GhostESP-Revival/GhostESP), commit
  `0156908de7ae6b9c73ffb380276d1d85c8852f13`.
- ESP-IDF v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`,
  matching the selected upstream build toolchain.
- Exact revisions are in `upstream.json`; registry components are pinned in
  `dependencies.lock`. Board changes are saved in `board.patch`.
- `prepare.py` fetches the pinned source, applies the patch and combines the
  upstream C3 configuration, round-board LVGL settings and `sdkconfig.board`.
  Editing the configuration inputs regenerates `build/sdkconfig`; put lasting
  changes in `sdkconfig.board` rather than relying on menuconfig edits.

The upstream checkout, toolchain, build and firmware output are ignored by Git.

## Hardware profile

| Signal | GPIO / setting |
|---|---|
| LCD SCK / MOSI | 6 / 7 |
| LCD CS / DC | 10 / 2 |
| LCD backlight | 3, active high, switched |
| LCD reset | Software reset; no reset GPIO |
| LCD bus | SPI2, mode 0, 20 MHz, BGR, inverted |
| Touch SDA / SCL | 4 / 5, I2C0 at 100 kHz |
| Touch reset / IRQ | 1 / 0; IRQ stays an input |
| Touch address | 0x15 |
| Console | Native USB Serial/JTAG |

Touch uses the reset timing and register settings tested in the existing Rust
firmware: 20 ms reset, 400 ms recovery, `0xFE=0x01`, `0xFA=0x60`, and an atomic
six-byte read from register `0x01`. Release, reserved-event and invalid-coordinate
frames cannot produce new contacts. The upstream two-sample debounce remains.

The port shares upstream's round home menu, status pill and toast behavior
without enabling CrowPanel power rails or encoder pins. The round layout uses
the actual circular aperture rather than a square viewport:

- Menu and favorites rows resize to the circle's available width as they scroll.
  Four-pixel clearance is measured over the entire visible row height. The wide
  middle can hold roughly 220 pixels; labels have six-pixel side padding. The
  default menu rows are 36 pixels high with two-pixel gaps; selected long labels
  scroll horizontally.
- All three keyboard pages keep their four rows between y=74 and y=199. Hidden
  layout spacers taper each row, preserving LVGL's native button geometry and
  the matching touch hit tests. The text field is separately fitted to its top
  band. The full QWERTY arrangement still has small touch targets on a 1.28-inch
  display; this layout prevents clipping without promising large keys.
- Up/Back/Down, terminal Back/Command and the unlock slider are kept inside the
  aperture. Terminal paragraphs use a stable inset viewport so scrolling does
  not constantly rewrap text. Legacy detail, game and overlay screens still
  require a complete visual audit.

`round_geometry.h` fits each horizontal band using integer circle geometry,
with no framebuffer, floating-point rendering or extra polling task. Host tests
check containment for all 26,796 interior bands and every keyboard page.

Two five-line RGB565 display buffers consume 4,800 bytes of internal DMA RAM;
allocation failure falls back to one buffer. A completion semaphore lets the UI
task yield while SPI DMA owns a buffer. The board's SPI transaction pool is four
entries rather than upstream's sixteen.
The 20 MHz display clock is conservative relative to the Rust firmware's 24 MHz.
The manual touch path handles BadBLE's Cancel button directly, and console
cursor/history echoes respect the disabled UART instead of emitting driver errors.

## Board additions and controls

These additions are specific to this ESP32-2424S012 build of the pinned upstream
revision. The existing GhostESP Wi-Fi/BLE scanners, RSSI trackers, WebUI terminal
and `/api/logs` endpoint are reused where noted.

### Round display and touch

The board driver resets the GC9A01 in software and reads CST816D/S touch frames
over I2C. Menu, favorites, keyboard, terminal, touch bar, status and unlock
controls have round-aperture layouts and matching hit tests. Rows shrink as they
approach the rim; a selected long menu label scrolls. The round layout also
disables the list-width wipe animation, which would fight row fitting. These
changes do not imply that every legacy detail or overlay screen has been
visually audited.

### On-device media and games

| Entry | What it adds | Console control |
|---|---|---|
| **Apps → Media → Copper Dream** | Procedural twister, copper bars and perspective floor over the current screen | `saver crew` |
| **Apps → Media → Neon Vortex** | Procedural plasma, rings and checker effect | `saver neon` |
| **Apps → Games → Orbital Vector** | Touch-steered orbital shooter; drag the rim, tap the centre to pause | `orbit on` / `orbit off` |
| **Apps → Games → Asteroids** | Touch steering and thrust, automatic fire and waves of splitting rocks | `asteroids on` / `asteroids off` |

The effects dismiss on touch and consume the waking gesture. Copper Dream is
selected by default and starts automatically after 15 seconds idle on the main
menu, while the screen is unlocked and lit. Reduced Motion suppresses automatic
start; locking, dimming or leaving that view stops it. `saver off` disables
automatic start for this boot, `saver auto` restores it, and `saver status`
reports activity and raster timing. `saver bench [crew|neon]` and `saver kernels`
are development diagnostics. Saver choices are not saved to NVS. The 20 fps
request period is a ceiling, not a measured display rate. See the
[effects and commands guide](SCREENSAVER.md).

Both games render LVGL vectors without a game framebuffer. Orbital Vector has
three lives, automatic fire and a session-only score; Asteroids has three lives,
automatic fire and advancing waves. Neither adds sound. See
[Orbital Vector](ORBITAL_VECTOR.md) and [Asteroids](ASTEROIDS.md) for controls and
implementation details.

### Radio survey and tracking

The WebUI **Recon** page collects snapshots from the existing Wi-Fi AP and BLE
advertisement result lists. Run and stop scans one radio at a time, then press
**Collect snapshot**. The board serves the current bounded results at
`GET /api/recon`, subject to the WebUI authentication setting; it rejects a
snapshot while a scan is active.
The browser keeps up to 128 distinct radio/address observations in local storage,
with first/last seen times from that browser's clock and a snapshot count.
**Download CSV** and **Download JSON** create files on the connected client.
The page's security reminder and script-tag example are inert display text.

**FOX HUNT** tracks a current result by RSSI using upstream's AP/BLE trackers.
It is offered for addresses in the latest Recon snapshot and on touch-screen
AP, station and BLE scan details. The console adds
`foxhunt ap <BSSID>`, `foxhunt ble <address>`, `foxhunt status` and
`foxhunt stop`. Tracking can interrupt the GhostNet AP and WebUI; status and
stop are best used through the device Terminal or USB while disconnected. RSSI
is a proximity clue, not a distance measurement. See the
[Recon and Fox Hunt guide](RECON_FOX_HUNT.md) for the workflow and limits.

`rfwatch` adds up to eight NVS-persisted exact-match rules for AP SSIDs/BSSIDs
and BLE names/addresses. After the existing `scanap`, `blescan -adv` or
`bledetect` scans have stopped, `rfwatch report` compares their retained results
with those rules and lists serial-module-name candidates from BLE detection.
Use `rfwatch add`, `rfwatch list`, `rfwatch remove <index>` and `rfwatch clear`
to manage rules. It has no background alert or dedicated touch view; a name
match is not proof of a device type. See [RF Watch](RF_WATCH.md) for exact syntax.

### Offline lab portal

`labportal [LAB-SSID]` starts the board-only **Per My Last Login** captive
portal with two fixed sandbox/test-token fixtures. It accepts no free-text
credentials or payment details. Submissions emit fixture IDs over the native
USB console; the optional `lab_poc.py` collector starts the portal and writes
synthetic JSON Lines evidence on the host. Events are not persisted on the
board. See the [lab PoC guide](LAB_POC.md) before running it.

### Memory and build changes

The profile reserves six static Wi-Fi TX buffers instead of upstream's sixteen,
uses a four-entry SPI transaction pool, and uses the two DMA strips described
above. The wired peer implementation is replaced with unsupported stubs so it
cannot allocate queues or claim LCD GPIO6/7. SD initialization and both SD mount
paths are disabled; BLE detection skips absent RGB LED calls. The compiled log
ceiling is INFO with WARN as the runtime default. Silent assertion/error checks
still abort on failure. The effects use integer/fixed-point raster kernels and
the build gives Copper Dream's source a local `-O3` setting; the project remains
otherwise size optimized. WebUI header generation also works when optional
Python minifiers are absent. See the [binary audit](BINARY_OPTIMISATION.md) and
[RISC-V measurements](RISC_V_OPTIMISATION.md) for measured trade-offs. These
changes improve headroom but do not establish long-running stability.

## Build

Prerequisites: Git, Python 3, CMake, Ninja, a host C compiler and the usual
ESP-IDF platform prerequisites. On macOS the host compiler comes from the
Xcode Command Line Tools. The first build needs network access to fetch the
pinned GhostESP source, ESP-IDF, C3 tools and registry components.

Run from this directory:

```sh
bash build.sh
```

`build.sh` calls `setup.py` when ESP-IDF is absent, and installs missing C3 tools
before compiling. Later builds reuse the local checkout and tools. To install
the toolchain ahead of the build, run `python3 setup.py`.
The default installation stays under `.tools/`.
An existing checkout of the exact pinned IDF can be selected with
`GHOSTESP_IDF_PATH` and its tools directory with `GHOSTESP_IDF_TOOLS_PATH`.

The build automatically checks the hardware configuration and partition fit,
runs `tests/touch_frame_test.c` and `tests/round_geometry_test.c`, and packages
these files under `dist/`:

- `ghostesp-esp32-2424s012-merged.bin`: bootloader, partition table and app;
  flash at **0x0**.
- `ghostesp-esp32-2424s012-app.bin`: app only; flash at **0x10000** only when
  the matching bootloader and partition table are already installed.
- Build configuration, source patch, revision and dependency locks, upstream
  license, manifest and SHA-256 checksums.

The last development build was 3,114,128 bytes in a 4,063,232-byte partition,
leaving 949,104 bytes (23%). There is one factory application partition;
this layout has no second application slot for OTA updates.

To repeat checks without rebuilding:

```sh
python3 verify.py
```

The native LVGL regression test repeats 100 menu create/rebuild/scroll/destroy
cycles and checks visible row geometry. The additional tests cover procedural
raster accuracy, screensaver lifecycle and disabled peer APIs. Timeouts detect stalls:

```sh
cmake -S tests -B build/layout-tests -G Ninja
cmake --build build/layout-tests --target round_layout_test round_demo_test round_saver_test peer_stub_test round_crew_test orbital_model_test orbital_view_test asteroids_model_test lab_poc_test rf_watch_test
ctest --test-dir build/layout-tests --output-on-failure
.tools/idf-tools/python_env/idf6.1_py3.15_env/bin/python tests/lab_poc_host_test.py
```

## Hardware validation

Before a flash, establish the target USB port and whether the current firmware
must be backed up. Flashing replaces that firmware. A merged image also covers
the NVS region with padding, so it does not preserve existing settings.

After choosing the correct port, build.sh forwards standard IDF actions:

```sh
bash build.sh -p /dev/cu.YOUR_BOARD flash monitor
```

For a raw backup, use the installed IDF Python environment's esptool and
`read-flash 0x0 0x400000 backup.bin` before writing. Close other serial clients.
Do not assume a device name remains the same after entering download mode.

First hardware acceptance checks:

1. Boot log identifies ESP32-C3 and 4 MB flash; no reset loop or allocation error.
2. LCD shows the round home menu with correct colors and orientation.
3. Touch log detects 0x15; taps, drags, release and idle have no phantom events.
4. Swipe between home entries, open a menu, scroll and return using Back.
5. Run a Wi-Fi AP scan and BLE advertisement scan; check results and heap logs.
6. Repeat navigation and scans, then power cycle and confirm saved settings.

These checks require the connected board and physical observation of its screen.

For a USB screenshot, use the installed IDF Python environment to run
`capture_screen.py --output build/screen.png` from this directory. It consumes
the existing RGB444 mirror protocol, saves a circularly masked PNG and the raw
square framebuffer, then turns mirroring off. `--interactive` keeps USB open
between captures: navigate physically, press Enter to capture, `:mem check` to
run a console command, and `q` to quit. Command responses are saved in a sibling
`.console.log`. `mem heaps` needs `:loglevel info` first; restore `:loglevel warn`
after diagnostics because the log-level setting persists.
On this macOS setup, opening the serial port can reset the board, so keep the
interactive session open while navigating. The helper waits for startup before
issuing mirror commands. `--port` overrides `/dev/tty.usbmodem101`.

## Saving data through the WebUI

There are two browser-side paths: `/api/logs` for retained terminal text, and
**Recon → Download CSV/JSON** for a structured survey of current AP/BLE scan
results collected into that browser. Neither path needs an SD card.

To save terminal text on the connected computer or phone:

1. Connect to the board's **GhostNet** Wi-Fi access point.
2. Open [the WebUI](http://192.168.4.1/) and log in if prompted.
3. Run the desired operation and wait for its text results to appear. The
   WebUI's terminal can also be used to view and copy the output.
4. In the same browser/session, open
   [the text-log endpoint](http://192.168.4.1/api/logs).
5. Use the browser's Save Page/Save As command to save the response as a text
   file, or copy its contents into a local text file. On a phone, the available
   save/share actions depend on the browser.

The endpoint sends `text/plain`; it does not automatically download an
attachment. This method uses the existing `/api/logs` handler in
`upstream/main/managers/ap_manager.c`, which streams the shared terminal history
in chunks. It does not create a file on the ESP32.

| Data or operation | Current support on this board |
|---|---|
| Retained terminal output and scan results printed there | Save the text-log response or copy the WebUI terminal |
| Structured Wi-Fi AP/BLE advertiser survey | In **Recon**, stop scans, collect a snapshot, then **Download CSV** or **Download JSON** on the client |
| Files page uploads/downloads and **Backup to SD** | Require mounted board storage; unavailable with this profile |
| Raw packet captures saved directly to the browser | Not implemented as a replacement for SD capture storage |
| Continuous, lossless logging to the browser | Not provided by the retained-history endpoint |

Terminal history is bounded and held in RAM. Older output can be discarded, and
a reset, power loss or clearing the logs loses the retained history. Save promptly; an
export contains only the text still retained, not every scan record or raw
packet observed. Radio operations can interrupt the access point and WebUI
connection. Reconnect before retrieving logs; reconnection cannot recover text
that has already been discarded.

Recon exports contain its browser-retained AP/BLE survey, not arbitrary terminal
output, station results, packet captures or every radio event. Its 128-row limit,
browser-local storage and client-clock timestamps are described above. There is
no one-click **Save Log** button; use the text-log endpoint for that data. Longer
or lossless recordings would need a streaming design that handles interruptions
and bounded buffering.

### Storage options and hardware limits

The board has no microSD slot, GPS or PSRAM. Its USB-C data connection uses the
ESP32-C3's fixed-function USB Serial/JTAG controller, which cannot act as a USB
host. A USB flash drive, hub or OTG adapter plugged into this port cannot provide
storage, and firmware cannot enable missing host hardware. See
[Espressif's ESP32-C3 USB documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html).

Internal flash could hold a small filesystem for scripts or other small files,
but none is configured as an SD replacement in this profile. The last measured
application was 3,114,128 bytes; the 949,104 bytes remaining in its application
partition are firmware growth space, not a mounted filesystem.
Adding file storage requires changing the partition layout and adapting the
GhostESP storage paths and feature checks. Existing settings already use NVS;
this does not make the SD file browser available.

For larger logs or captures, USB serial or Wi-Fi streaming to a computer is a
possible development path. General capture streaming still needs implementation;
the existing text-log endpoint retrieves retained history, while Recon exports
only its bounded AP/BLE survey. For removable USB storage, an ESP32-S3 board
with USB-OTG host wiring and a suitable 5 V supply
is a better starting point. Espressif provides a
[USB mass-storage host example](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/usb/host/msc/README.md);
GhostESP integration would still be required. An S3 board with PSRAM and microSD
is a better fit for the full GhostESP feature set.

Storage capacity and working RAM are separate constraints. This port's memory
reductions improved free internal RAM during diagnostics from about 4.7 KB to
22.2 KB, but later activity recorded a minimum of just 628 bytes (DMA minimum
60 bytes). Heap integrity passed; the reported freeze's root cause and sustained
stability remain unconfirmed. WebUI export or additional storage does not add RAM.

## Scope and limitations

Wi-Fi and BLE features use the upstream C3 implementations. GPS, SD storage,
native SD apps, GhostScript, wired peer UART, RGB LED, battery sensing, encoder,
joystick and external radio hardware are disabled. The peer UART's usual GPIO6/7
assignments would conflict with the LCD. Both normal and just-in-time SD mount
entry points return unsupported before touching GPIOs.

Upstream menus and commands may still offer operations that need absent storage
or external hardware. Capture saving, wardriving, SD asset packs and downloadable
SD apps are outside this profile. Native USB keyboard/mouse emulation is also
outside the C3 profile. The target is a compact touch-operated Wi-Fi/BLE tool;
it is not feature-equivalent to a GhostESP board with PSRAM and SD storage.

The selected upstream emits compiler and Kconfig deprecation warnings; the
successful build is not warning-free. Keep the source revision pinned while
bringing up hardware, then review upstream changes before updating it.
