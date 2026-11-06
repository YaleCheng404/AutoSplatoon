# Hardware and connection troubleshooting

<a name="开发板需要支持经典蓝牙"></a>

## ESP32 board requirements

Use an original ESP32 board with USB-to-serial, such as a WROOM, WROVER, or PICO board supported upstream. The bundled UARTSwitchCon 1.2 firmware uses Bluetooth Classic and UART0 at 19200 baud. ESP32-S2, S3, C3, and C6 are unsupported. See the [firmware source and checksum](../firmware/README.md) and the [upstream project](https://github.com/nullstalgia/UARTSwitchCon/tree/1.2).

A USB data cable usually connects directly to the board's built-in USB-to-serial interface. For custom boards or an external adapter, confirm UART0, the common ground, and signal levels in the board and firmware documentation. The UART2 firmware uses different wiring.

<a name="首次使用需要烧录和配对"></a>

## Flash and pair for the first time

Follow the [first drawing steps](../README.md#first-drawing) to flash the board, connect, pair, and calibrate the game cursor. On Windows the serial device description may contain CH340, CH9102, or CP210x. Confirm it against your board.

The bundled esptool detects the original ESP32 before writing the verified combined firmware to `0x0`. Close the flashing dialog when the log reports success. You can reuse the installed firmware when a software upgrade bundles the same version.

Manual Controls are disabled during drawing, including while paused. See [Manual controls](../README.md#manual-controls) for pairing and calibration buttons.

<a name="连接问题可以逐项排查"></a>

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| The serial port is missing | Check the USB data cable, port, and driver. Use Device Manager on Windows. On Linux, check serial device permissions and sign out and in after changing groups. |
| The serial port will not open | Close serial monitors, Arduino IDE, or other apps using the port. Check the selected device and your permissions. |
| esptool cannot connect | Check the port and chip. Hold BOOT and retry. Follow the board instructions for RESET and keep the original error from the log. |
| The chip is unsupported | Use an original ESP32. Keep the chip check enabled. |
| Serial is connected but Start Drawing is disabled | Pair on the Switch. Drawing becomes available once the application receives HID reports. |
| Controller reports stop or pairing fails again | If reports stop for over five seconds, the app stops drawing. Click **Disconnect**, then **Connect** to restart the board and repeat the handshake. Pair on the Switch and recalibrate the cursor. |
| Auto-reset is unavailable | The port cannot drive the reset line. AutoSplatoon still tries the normal handshake. If pairing fails, disconnect, press RST on the board, and reconnect. |
| The drawing is offset | Stop and check that the console cursor matches the start row and column. A scheduling delay can also pause drawing. Check the console and the app before resuming. |
| The preview has gray areas or cropped edges | Gray marks the area outside the canvas. Export and drawing use the canvas inside the blue outline. Drag, zoom, or reset the image in **Composition**. |
| DLLs are missing after upgrading | Extract the full Windows ZIP. Keep the executable, Qt runtime and plugins, tools, and other resources together. |

Use Device Manager on Windows or System Information on macOS to identify the USB-to-serial chip. Download the [Silicon Labs CP210x driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) or [WCH CH34x driver](https://www.wch.cn/downloads/CH341SER_EXE.html) as appropriate. On Linux, check the owning group for `/dev/ttyUSB*` or `/dev/ttyACM*`. On Debian or Ubuntu, when the device is owned by `dialout`, run `sudo usermod -aG dialout "$USER"` and sign out and back in.

<a name="自动复位与验证范围"></a>

## Auto-reset and validation

Each connection attempts to reboot the ESP32 through the USB-to-serial DTR and RTS lines. A successful reboot drops the existing Bluetooth connection. The board needs the corresponding auto-reset circuit. Confirm recovery by pairing on the console and checking the controller buttons.

See [validation](validation.md) for the tested YD-ESP32 and Switch 2 combination and the scope of the recovery checks.
