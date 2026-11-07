# Validation and test procedures

Software tests cover serial and flashing error paths. Board reset, console pairing, and controller response require hardware tests.

<a name="已完成的连接恢复实测"></a>

## Measured connection recovery

On 2026-10-22, a user tested auto-reset on connect in a Windows test build.

| Check | Result |
| --- | --- |
| Board | YD-ESP32, CH340K, ESP32-D0WD-V3 v3.1 |
| Console | Switch 2, system version 23.0.1 |
| First connection | Pairing succeeded. Directional buttons and A responded. |
| Connection after reflashing the same firmware | Passed |
| Recovery after using a Joy-Con in-game | Disconnect and reconnect in the app recovered pairing and button response in 5 of 5 trials, without pressing RST or unplugging USB. |

Using a Joy-Con in-game could trigger immediate loss of controller communication. All five trials used an ESP32 reset to recover. Firmware recovery without a reset remains unverified.

These results apply to the hardware in the table. Full 320 × 120 drawing and saving, long runs, USB removal, timeouts, and compatibility with other boards need checks on the corresponding software and hardware.

<a name="自动测试"></a>

## Automated tests

CTest checks drawing plans for single pixels, edge columns, empty rows, custom starts, and save at completion. It also checks pause, resume, stop, and input release, controller packets and CRC, and the firmware checksum.

All four image presets are checked for the expected size and black-and-white output. Tests cover transparency, small images, preview scaling, and preservation of dark strokes and outlines by Pencil Sketch. Rapid imports and sample switches keep the latest result. Samples reset image settings and preserve drawing settings. Reduce motion finishes any preset explanation animation already in progress.

Connection tests check disabled controls while connecting, duplicate clicks, and cleanup of pending handshakes and old HID state after disconnect or failure. Simulated flashing covers success, unsupported chip, write failure, and cancellation.

The core suite runs in separate English and Simplified Chinese processes. It checks language selection and fallback, the first-run guide, and that changing the preference leaves the current session and image intact. CI runs the suite at 100%, 125%, 150%, 200%, 250%, and 300% display scaling. See [GitHub Actions](https://github.com/YaleCheng404/AutoSplatoon/actions/workflows/build.yml).

<a name="发行包检查"></a>

## Package checks

After extracting or mounting a package, run `tools/verify_package.py` to check the firmware SHA-256, esptool version, plugins, and documentation, including the Chinese user guides. Use `tools/check_gui.py` to run startup checks with SDK search paths removed. Linux CI also checks X11, Wayland, and startup in Ubuntu and Debian containers.

`AutoSplatoon --smoke-test` processes a test image and exits. Set `AUTOSPLATOON_SMOKE_OUTPUT` to an existing directory to save the window screenshot, 320 × 120 image, language, and drawing plan checksum. The app runs offline.

<a name="图像与界面检查"></a>

## Image and interface checks

Set `AUTOSPLATOON_QA_OUTPUT` to save core test artifacts, including preset, photo, and binary outputs. Use them to compare pixels across dependency or algorithm changes.

Packaged GUI checks cover 18 resolution and scale combinations per language. Both languages must render their own labels and produce the same image and drawing plan. On real desktops, check 1080p, 2K, 4K, and movement between monitors. Controls must remain reachable by scrolling in small windows. Check X11 and Wayland separately.

The [example generator](building.md#generate-example-images) compares the workbench and processor results for all three samples and four presets. It verifies 320 × 120 binary output and records [source and output hashes](images/examples/examples.json). Check that the task strip stays at the bottom while scrolling the canvas and inspector.

<a name="实机复测顺序"></a>

## Hardware retest procedure

Flash and pair first. Calibrate the cursor and test a few pixels. Then check pause, resume, release on stop, USB removal, and timeout handling. Finish with a complete drawing and save.

To retest recovery, run five independent trials. In each trial, use a Joy-Con in the game to trigger a disconnect, confirm pairing fails again, and recover using only Disconnect and Connect in the app. Check the directional buttons and A on the console. Record the board, USB-to-serial chip, app and console system versions, and the executable path and SHA-256.
