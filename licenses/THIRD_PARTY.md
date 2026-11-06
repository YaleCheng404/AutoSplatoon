# Third-party software and sources

AutoSplatoon is distributed under GPL-3.0; see LICENSE.

| Component | Source | License / notice |
|---|---|---|
| Original application | [jiangotto/AutoSplatoon](https://github.com/jiangotto/AutoSplatoon), [wang-ji-yuan fork](https://github.com/wang-ji-yuan/AutoSplatoon) | GPL-3.0, project LICENSE |
| Action queue and serial transport | [zhougz520 fork, 1309d77](https://github.com/zhougz520/AutoSplatoon/tree/1309d77af0210cd951d49d7aaf4e2b1e2baf7d1c) | GPL-3.0, project LICENSE |
| Manual control layout and icons | [ClubchatGames, af2bdac](https://github.com/nullstalgia/ClubchatGames/tree/af2bdace90e0f0641aa4e21e0a24ca998a676ad0) | GPL-3.0, project LICENSE |
| Qt | Official SDK pinned in dependencies.lock.json | LGPL-3.0 / GPL-3.0, Qt-LGPL-3.0.txt and SDK notices in Qt-SBOM |
| OpenCV | [5.0.0](https://github.com/opencv/opencv/tree/5.0.0), core/imgproc/photo and required geometry/flann modules | Apache-2.0, OpenCV.txt; FLANN notices in OpenCV-FLANN.txt; dependency notices installed from OpenCV's licenses component in OpenCV-third-party |
| Application typeface | [Adobe Source Han Sans CN Variable, 2.005R](https://github.com/adobe-fonts/source-han-sans/releases/tag/2.005R) | SIL OFL-1.1, SourceHanSans-OFL.txt |
| esptool | [Official standalone releases](https://github.com/espressif/esptool/releases), version pinned in dependencies.lock.json | GPL-2.0-or-later, esptool.txt |
| UARTSwitchCon firmware | [1.2](https://github.com/nullstalgia/UARTSwitchCon/tree/1.2), PRO-UART0.bin | GPL-3.0, UARTSwitchCon.txt |
| Squid and ink artwork | [Slidoon by Mzyy94](https://github.com/mzyy94/Slidoon), [author's permission for the artwork without the Finish banner](https://www.mzyy94.com/blog/2015/12/08/packetoon-after-that/) | CC BY-SA 4.0; recolored artwork and its adaptations under the same license |
| Ink Splash | [3070260, uploaded by oksmith](https://openclipart.org/detail/298859/ink-splash) | CC0 1.0; recolored |
| Pattern Pack | [Kenney](https://opengameart.org/content/pattern-pack) | CC0 1.0; two original tiles |
| Controller artwork and button prompts | [Zacksly](https://zacksly.itch.io/switch-button-icons-and-controls), individual vectors sourced from Inkipedia | CC BY 3.0 |
| Paint Roller | [Delapouite, Game-icons.net](https://game-icons.net/1x1/delapouite/paint-roller.html) | CC BY 3.0; recolored |

See [ARTWORK.json](ARTWORK.json) for file-level credit, source checksums, modifications and use. The CC BY-SA 4.0 attribution and license also apply to the Slidoon adaptations in the squid and ink example images and to their processing results. Splatoon and Nintendo Switch are marks of Nintendo.

Algorithm references: [OpenCV thresholding](https://docs.opencv.org/5.0.0/d7/d4d/tutorial_py_thresholding.html) and [pencilSketch](https://docs.opencv.org/5.0.0/df/dac/group__photo__render.html). Manual control uses Qt Widgets layouts and [pressed/released signals](https://doc.qt.io/qt-6/qabstractbutton.html). Deployment uses [Qt's CMake deployment API](https://doc.qt.io/qt-6/qt-generate-deploy-app-script.html) and the official linuxdeploy Qt plugin.

Build-only tools (Python, CMake, Ninja, 7-Zip, linuxdeploy) are not installed in application packages. Download URLs and SHA-256 values are recorded in dependencies.lock.json, Python build packages in tools/requirements-build.txt, and GitHub Action commit IDs in .github/workflows/build.yml.
