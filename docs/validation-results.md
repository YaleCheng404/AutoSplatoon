# 2026-10-01 验证记录

环境：Windows x64（系统版本 10.0.26300），Qt 6.12.0 官方 SDK、MinGW 15.1.0、CMake 4.4.3、Ninja 1.13.2、OpenCV 4.14.0 静态 core/imgproc/photo。最终构建目录为 `build/native-windows`；默认开发预设为 `build/release`。

| 检查 | 本次结果 |
|---|---|
| 本地 Release 编译 | 通过；原生 C++ 编译和链接完成 |
| QtTest 核心测试 | 初版 23 项通过；1.0.2 为 26 项通过、0 失败、0 跳过；初版六档缩放各运行一次 |
| 图像测试 | 照片、文字/线形、渐变、透明输入、1×1 纯色；四预设尺寸/二值/确定性，反色与独立画布检查通过 |
| 照片视觉检查 | 对照 OpenCV fruits.jpg；默认线稿保留轮廓；铅笔默认曝光修正，清理小散点，加入过黑回归检查；见 images/photo-comparison.png |
| 显示模拟 | 1920×1080、2560×1440、3840×2160，各六档缩放，共 18 案例通过；窗口客户端位于可用屏幕内，极小窗口可滚动 |
| 输出一致性 | 18 案例输出 PNG SHA-256 相同：5d654034631f23fd6a762833ca7f1575180f1e97089a44deaa57198273adc43e；见 gui-results.json |
| 中文字体 | Windows 系统字体截图正常；不提供系统字体目录时，内置 Noto Sans CJK 回退及中文字形检查通过 |
| 烧录进程模拟 | 成功、芯片探测失败、不继续写入、写入失败与取消通过；测试程序核对端口 TEST_PORT、esp32、0x0 和固件长度 |
| 固件与工具 | PRO-UART0.bin SHA-256 核对通过；包内 esptool version 输出 5.4.0 |
| 脱离开发环境启动 | 清除 Qt/编译器 PATH，只保留 Windows/System32 后，部署目录成功启动并处理图片；没有联网调用 |
| Windows ZIP | 已使用 CPack 生成，并解压重新检查固件、esptool 与脱离开发 PATH 的启动；程序启动与内置测试图处理通过 |

初次运行测试目录曾缺少 Qt DLL 并产生系统弹窗，已通过部署 Qt 依赖及匹配编译器运行库修复。后续验证工具设置 Windows SetErrorMode，避免依赖错误弹窗打断桌面。初次 offscreen 截图缺少系统字体，现使用系统字体目录并提供内置中文字体回退。1080p 高缩放下的最小高度超屏问题已由整体滚动容器修复。

**初次 Windows 本地验证未涵盖**：真实 2K/4K 显示器跨屏移动；macOS ARM64 原生构建/DMG 启动；Linux 原生 AppImage、Ubuntu 24.04/Debian 12 与 X11/Wayland；ESP32 实机烧录、USB 断连/响应超时、Switch 配对、完整绘图与保存。对应 CI 或实机验收步骤已提供，但不能把配置流程视为运行通过。照片对比只覆盖一个标准素材，更多题材仍需视觉验收。

## 1.0.1 手动控制误超时修复

用户实机反馈：烧录内置固件、连接后进入手动控制，出现“设备响应超时，绘图已停止”。核对 UARTSwitchCon 1.2 的 `send_buttons()` 源码，0x90 来自蓝牙 HID 定时报告，不是串口输入包的逐包确认；未连接 Switch 时没有该报告。因此移除逐包确认队列，分别维护串口握手与 HID 报告状态。

配对前保留手动控制；收到 HID 报告后启用自动绘图；已经有报告后中断超过 5 秒，停止绘图但保留串口供重新配对。接收窗口不再被连续报告不断重置，避免已配对设备的握手被饿死。同时修复驱动链接显示成 HTML 字符串的问题。

回归结果：26 项测试通过，0 失败、0 跳过。新增模拟覆盖未配对长期静默、主动周期报告、通信中断/恢复、NACK、配对阶段手动按钮可用/绘图按钮禁用、报告丢失停止动作，以及连续报告不阻塞握手接收窗口。这些是协议状态与界面的模拟检查，修复版随后由用户复验，反馈无问题（见下方反馈范围）。固件文件和哈希不变，更新软件不需要重刷固件。

## 1.0.2 画布边框与用户复验

两个 CanvasView 均在预览绘制层显示蓝色画布边框、灰色外围及超出区域遮罩；不向 composition() 合成结果添加任何边框。26 项回归测试通过；解压后的 Windows 发行包在移除开发 PATH 后完成 18 个分辨率／缩放 offscreen 案例，输出哈希仍为上表数值。中文与边框截图目视检查通过，内置固件及 esptool 校验通过。

2026-10-01，用户针对 1.0.2 回复“经测试没有问题了”。此反馈记录为用户实机复验通过，涵盖此前报告的问题得到复验；未提供板卡型号、Switch/系统版本或详细测试清单，不据此宣称所有断连、取消、长时间绘图与保存情形均通过。真实跨屏以及 macOS/Linux 桌面体验仍需对应环境验证。

## GitHub 原生构建与 Release

发布流程包含 Windows x64、macOS ARM64、Linux x64 原生构建，Linux Ubuntu 24.04 / Debian 12 容器启动，以及四份产物的完整性与 SHA-256 清单检查。各项执行结果以对应标签的 GitHub Actions 日志为准；此文档中的本地结果不代表云端任务已通过。

### 首次跨平台云端结果

[GitHub Actions 36881974088](https://github.com/YaleCheng404/AutoSplatoon/actions/runs/36881974088) 的 Windows x64、macOS ARM64、Linux x64 三个 build 任务均通过：原生编译、26 项核心测试 × 六档缩放、包内固件和 esptool、18 个显示模拟案例及部署启动。Windows ZIP、macOS DMG、Linux AppImage 和目录压缩包已生成；Linux 的 X11、Wayland 启动也通过。

该轮精简 Ubuntu 24.04 / Debian 12 容器缺少操作系统图形栈的 libOpenGL.so.0，检查失败。后续任务已补齐系统 libopengl0；正式 Release 仍要求两个容器检查成功。最新官方 Actions 固定为 checkout v7.0.1、setup-python v7.0.0、upload-artifact v7.0.1、download-artifact v8.0.1，使用 Node.js 24。完整发布结果请查看 [v1.0.2 对应工作流](https://github.com/YaleCheng404/AutoSplatoon/actions/workflows/build.yml) 和 [Release](https://github.com/YaleCheng404/AutoSplatoon/releases/tag/v1.0.2)，不得将首轮容器失败标为通过。

### v1.0.2 容器阻止发布与 1.0.3 修复

v1.0.2 正式任务 [36883580496](https://github.com/YaleCheng404/AutoSplatoon/actions/runs/36883580496) 的三个 build 任务通过，但 Debian 容器找不到 offscreen 插件，发布被阻止。发现 Linux 构建机的 LD_LIBRARY_PATH 会使此前启动检查使用 SDK 库和插件，因此首轮 Linux 的显示及 X11/Wayland 检查不能作为自包含部署成功的证据。

1.0.3 改用官方 EXTRA_PLATFORM_PLUGINS / EXTRA_QT_MODULES 参数，并在 GUI 与 X11/Wayland 启动测试移除 SDK 搜索路径。v1.0.2 标签保留，不覆盖；完整发行以 [v1.0.3](https://github.com/YaleCheng404/AutoSplatoon/releases/tag/v1.0.3) 及其 Actions 执行结果为准。Windows 1.0.2 的用户实机反馈继续保留，1.0.3 的 Windows 功能保持一致，升级无需重刷。
