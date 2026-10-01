# 内置固件

- 来源：[nullstalgia/UARTSwitchCon 1.2](https://github.com/nullstalgia/UARTSwitchCon/releases/tag/1.2)
- 发布资源：`ESP32.Binaries.zip`，文件：`PRO-UART0.bin`
- 文件大小：648768 字节
- SHA-256：`d8b2e49b221bb363ef1d72908582ce48690e3b9538500aeb77f470b433b66039`
- 烧录地址：`0x0`，合并固件包含 bootloader / partition table / application。
- 通信：UART0，19200 baud，Chocolate 握手，Pro Controller。
- 许可证：GPL-3.0，完整文本见 `licenses/UARTSwitchCon.txt`。

仅用于上游支持的传统 ESP32（带 Bluetooth Classic，例如 WROOM/WROVER/PICO）。
不要用于 ESP32-S2/S3/C3/C6。软件使用 esptool `--chip esp32` 在写入前验证目标。
发布包内固件在 Qt 资源中嵌入，并额外附带可检查的 `.bin` 文件。

此固件发布说明提及 Switch 12.0.0 及之后系统的修复；这不是对今天全部主机系统版本的实机兼容性证明。
