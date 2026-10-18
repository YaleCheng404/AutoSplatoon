# Third-party software and sources

AutoSplatoon is distributed under GPL-3.0; see LICENSE.

| Component | Source | License / notice |
|---|---|---|
| Original application | [jiangotto/AutoSplatoon](https://github.com/jiangotto/AutoSplatoon), [wang-ji-yuan fork](https://github.com/wang-ji-yuan/AutoSplatoon) | GPL-3.0, project LICENSE |
| Action queue and serial transport | [zhougz520 fork, 1309d77](https://github.com/zhougz520/AutoSplatoon/tree/1309d77af0210cd951d49d7aaf4e2b1e2baf7d1c) | GPL-3.0, project LICENSE |
| Manual control layout and icons | [ClubchatGames, af2bdac](https://github.com/nullstalgia/ClubchatGames/tree/af2bdace90e0f0641aa4e21e0a24ca998a676ad0) | GPL-3.0, project LICENSE |
| Qt | Official SDK pinned in dependencies.lock.json | LGPL-3.0 / GPL-3.0, Qt-LGPL-3.0.txt and SDK notices in Qt-SBOM |
| OpenCV | [5.0.0](https://github.com/opencv/opencv/tree/5.0.0), core/imgproc/photo and required geometry/flann modules | Apache-2.0, OpenCV.txt; FLANN notices in OpenCV-FLANN.txt; dependency notices installed from OpenCV's licenses component in OpenCV-third-party |
| Chinese glyph fallback | [Noto Sans CJK SC, Sans2.004](https://github.com/notofonts/noto-cjk/tree/Sans2.004) | SIL OFL-1.1, Noto-CJK-OFL.txt |
| esptool | [Official standalone releases](https://github.com/espressif/esptool/releases), version pinned in dependencies.lock.json | GPL-2.0-or-later, esptool.txt |
| UARTSwitchCon firmware | [1.2](https://github.com/nullstalgia/UARTSwitchCon/tree/1.2), PRO-UART0.bin | GPL-3.0, UARTSwitchCon.txt |

Algorithm references: [OpenCV thresholding](https://docs.opencv.org/5.0.0/d7/d4d/tutorial_py_thresholding.html) and [pencilSketch](https://docs.opencv.org/5.0.0/df/dac/group__photo__render.html). Manual control uses Qt Widgets layouts and [pressed/released signals](https://doc.qt.io/qt-6/qabstractbutton.html). Deployment uses [Qt's CMake deployment API](https://doc.qt.io/qt-6/qt-generate-deploy-app-script.html) and the official linuxdeploy Qt plugin.

Build-only tools (Python, CMake, Ninja, 7-Zip, linuxdeploy) are not installed in application packages. Download URLs and SHA-256 values are recorded in dependencies.lock.json, Python build packages in tools/requirements-build.txt, and GitHub Action commit IDs in .github/workflows/build.yml.
