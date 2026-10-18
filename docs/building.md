# 构建与发布

运行程序无需 Python、Qt SDK 或 OpenCV 开发库。构建使用 C++17、CMake、Ninja 和 Python 3.15。SDK、编译器、解压工具及下载资产固定在 `dependencies.lock.json`；Python 构建包固定在 `tools/requirements-build.txt`。

## 本地构建

安装 Python 3.15，然后执行以下 Windows x64 示例。`install_qt.py` 下载并校验官方 Qt、匹配的 MinGW 和 7-Zip；无需单独安装解压工具。

```powershell
python -m venv .deps/venv
.deps/venv/Scripts/python -m pip install -r tools/requirements-build.txt
.deps/venv/Scripts/python tools/install_qt.py --platform windows-amd64
$env:PATH = "$PWD/.deps/venv/Scripts;$PWD/.deps/compiler/mingw64/bin;$PWD/.deps/qt/bin;$env:PATH"
python tools/fetch_assets.py --platform windows-amd64 --output build/release/assets
cmake --preset release -DCMAKE_PREFIX_PATH="$PWD/.deps/qt" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build --preset release --parallel 3
windeployqt --release --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler --include-plugins qoffscreen build/release/autosplatoon_tests.exe
ctest --preset release
cmake --install build/release --prefix "$PWD/dist/stage" --component Unspecified
python tools/verify_package.py dist/stage --platform windows-amd64
cpack --config build/release/CPackConfig.cmake -B dist
```

macOS 使用 `macos-arm64` 和 Xcode/Apple Clang，配置时指定 `-DCMAKE_OSX_ARCHITECTURES=arm64`，最低系统版本为 14.4。Linux 使用 `linux-amd64` 和 GCC，构建基线为 Ubuntu 22.04。两者将虚拟环境的 `bin` 和 Qt 的 `bin` 加入 PATH，省略 Windows 编译器参数和 windeployqt。Linux 构建所需系统库清单见工作流；执行 `python tools/package_linux.py --qt /absolute/path/.deps/qt` 生成 AppImage 和目录包。

OpenCV 从锁定源码静态构建 core、imgproc、photo 及其必需的 geometry/flann 依赖。图片格式由 Qt 插件提供。Qt 动态链接，发行包同时携带 Qt SBOM、OpenCV 第三方声明、固件、字体和烧录工具许可证。

## 验证

[验证方法](validation.md)说明核心测试、显示模拟及实机检查范围。使用最终归档解压或挂载后的文件执行 `verify_package.py` 和 `check_gui.py`，而非只检查构建目录。检查须清除 Qt SDK 搜索变量；GUI 工具已执行该隔离。

Linux 使用官方 linuxdeploy Qt 插件的 `EXTRA_PLATFORM_PLUGINS` 和 `EXTRA_QT_MODULES` 部署 offscreen、Wayland 与 SVG。CI 另执行 X11/Wayland 启动和 Ubuntu 24.04、Debian 12 容器检查。

## 发布

1. 更新 CMake 项目版本，编写该版本的 `docs/release-notes/v<版本>.md`；应用版本由构建配置传入。
2. 提交源码、依赖锁、资源和文档，推送 `master`。build、dist、.deps 不提交。
3. 推送与项目版本相符的 `v<版本>` 标签。三平台构建、测试、最终包验证及 Linux 容器检查全部通过后，工作流统一发布 Windows ZIP、macOS DMG、Linux AppImage、目录压缩包和 `SHA256SUMS.txt`。
4. 下载校验清单，对照各资产 SHA-256；保持已发布标签不可变。

发布任务使用 GitHub Actions 自带令牌，无需额外发布密钥。目前应用未配置正式代码签名或 macOS 公证。历史版本说明和验证日志保存在 Git、GitHub Releases 与 Actions 中。

确认新版归档可用后，可清理旧 build、临时安装目录、旧版安装包和下载缓存。保留当前发行包及必要日志；.deps 中的 SDK、编译器和虚拟环境可继续使用。
