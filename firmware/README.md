# 内置固件

| 项目 | 内容 |
| --- | --- |
| 来源 | [nullstalgia/UARTSwitchCon 1.2](https://github.com/nullstalgia/UARTSwitchCon/releases/tag/1.2) |
| 发布资源 | `ESP32.Binaries.zip` 中的 `PRO-UART0.bin` |
| 文件大小 | 648768 字节 |
| SHA-256 | `d8b2e49b221bb363ef1d72908582ce48690e3b9538500aeb77f470b433b66039` |
| 烧录地址 | `0x0`，合并固件包含 bootloader、partition table 和 application |
| 通信 | UART0，19200 波特率，Chocolate 握手，Pro Controller |
| 许可证 | GPL-3.0，完整文本见 `licenses/UARTSwitchCon.txt` |

该固件适用于上游支持的传统 ESP32，例如带 Bluetooth Classic 的 WROOM、WROVER 和 PICO。ESP32-S2、S3、C3、C6 不适用。软件通过 esptool 的 `--chip esp32` 参数检查芯片后再写入。

发行包将固件嵌入 Qt 资源，并附带可单独校验的 `.bin` 文件。升级软件时，只要内置固件未变，就无需重新烧录。

上游发布说明包含 Switch 12.0.0 及后续系统的兼容修复。当前已实测的设备组合和功能范围见 [验证说明](../docs/validation.md)。
