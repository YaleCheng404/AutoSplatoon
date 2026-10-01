# Third-party software and sources

AutoSplatoon retains the original GPL-3.0 license and attribution.

| Component | Version / source | License / notice |
|---|---|---|
| Original application | [jiangotto/AutoSplatoon](https://github.com/jiangotto/AutoSplatoon), [wang-ji-yuan fork](https://github.com/wang-ji-yuan/AutoSplatoon) | GPL-3.0, project LICENSE |
| Action queue and transport reference | [zhougz520 fork](https://github.com/zhougz520/AutoSplatoon/tree/1309d77af0210cd951d49d7aaf4e2b1e2baf7d1c) | GPL-3.0; queue adapted, planner and transport simplified |
| Qt | 6.12.0, official SDK | LGPL-3.0 / GPL-3.0; see Qt notices shipped from SDK |
| OpenCV | 4.14.0, core/imgproc/photo only | Apache-2.0, OpenCV.txt |
| Chinese glyph fallback | [Noto Sans CJK SC, Sans2.004](https://github.com/notofonts/noto-cjk/tree/Sans2.004) | SIL OFL-1.1, Noto-CJK-OFL.txt; unmodified font embedded for missing system glyphs |
| esptool | 5.4.0, official standalone executable | GPL-2.0-or-later, esptool.txt; [source](https://github.com/espressif/esptool/tree/v5.4.0) |
| UARTSwitchCon firmware | 1.2, PRO-UART0.bin | GPL-3.0, UARTSwitchCon.txt; [source](https://github.com/nullstalgia/UARTSwitchCon) |

The action player preserves the reference fork's frame-boundary pause, generation
guard against reentrant stop, and timing-overrun protection. Qt signal connections
are updated to typed connections. The planner keeps only full-width serpentine
scanning and omits RLE / drawing-while-moving strategies.

Algorithm and workflow references: [OpenCV image thresholding](https://docs.opencv.org/4.x/d7/d4d/tutorial_py_thresholding.html),
[OpenCV pencilSketch](https://docs.opencv.org/4.x/df/dac/group__photo__render.html),
[Exception0x0194 greedy fork](https://github.com/Exception0x0194/AutoSplatoon),
[splatplost](https://github.com/Victrid/splatplost), [img2splat](https://github.com/JonathanNye/img2splat),
[amowu desktop/resource fork](https://github.com/amowu/AutoSplatoon).
No code from the greedy / Bluetooth-only plotters is incorporated.

SDK and build-only release archive URLs, SHA-256 hashes, tool versions and GitHub
Action commit IDs are recorded in dependencies.lock.json. The build utilities
linuxdeploy and linuxdeploy-plugin-qt are not installed in application packages.
