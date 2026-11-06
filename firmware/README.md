# Bundled firmware

| Item | Details |
| --- | --- |
| Source | [nullstalgia/UARTSwitchCon 1.2](https://github.com/nullstalgia/UARTSwitchCon/releases/tag/1.2) |
| Release asset | `PRO-UART0.bin` from `ESP32.Binaries.zip` |
| Size | 648768 bytes |
| SHA-256 | `d8b2e49b221bb363ef1d72908582ce48690e3b9538500aeb77f470b433b66039` |
| Flash address | `0x0`. The combined image includes the bootloader, partition table, and application. |
| Communication | UART0, 19200 baud, Chocolate handshake, Pro Controller |
| License | GPL-3.0. See [the full text](../licenses/UARTSwitchCon.txt). |

The firmware supports original ESP32 variants supported upstream, including WROOM, WROVER, and PICO with Bluetooth Classic. ESP32-S2, S3, C3, and C6 are unsupported. The app uses esptool's `--chip esp32` check before writing.

Packages embed the firmware as a Qt resource and include a separate `.bin` for verification. Reuse the installed firmware when upgrading to a package with the same image.

The upstream release notes list compatibility fixes for Switch system version 12.0.0 and later. See [validation](../docs/validation.md) for the tested hardware and procedures.
