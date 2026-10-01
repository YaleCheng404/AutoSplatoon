# 构建与发行

运行程序不需要 Python、Qt SDK 或 OpenCV 开发库。以下工具仅用于构建机器。

依赖及下载 SHA-256 固定在 `dependencies.lock.json`：Qt 6.12.0、OpenCV 4.14.0、esptool 5.4.0、CMake 4.4.3、Ninja 1.13.2。OpenCV 仅静态编译 core/imgproc/photo；图片格式使用 Qt 插件。

## 本地构建

安装 Python 3.12、7z 和对应 C++ 编译器。Windows 安装脚本同时安装并校验 MinGW x64 15.1（Qt 6.12 对应版本），macOS 使用 Xcode/Apple Clang，Linux 使用 GCC。Linux 基线 Ubuntu 22.04，需要图形系统的 GL/EGL、XCB、Wayland 和字体系统库。

```sh
python -m pip install -r tools/requirements-build.txt
python tools/install_qt.py --platform windows-amd64
python tools/fetch_assets.py --platform windows-amd64 --output build/release/assets
cmake --preset release -DCMAKE_PREFIX_PATH="/absolute/path/AutoSplatoon/.deps/qt"
cmake --build --preset release --parallel 3
windeployqt --release --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler --include-plugins qoffscreen build/release/autosplatoon_tests.exe
ctest --preset release
cmake --install build/release --prefix "/absolute/path/AutoSplatoon/dist/stage" --component Unspecified
python tools/verify_package.py dist/stage --platform windows-amd64
```

以上示例使用 Windows 平台，需将 `.deps/compiler/mingw64/bin`、`.deps/qt/bin` 依次加入构建 PATH，并向 CMake 指定 `-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`。macOS 将平台替换为 `macos-arm64`，移除 windeployqt 命令，指定 `-DCMAKE_OSX_ARCHITECTURES=arm64`，部署最低版本 14.4。Linux 平台为 `linux-amd64`，同样移除 windeployqt 命令。

Windows/macOS 在 build/release 执行 `cpack` 生成 ZIP/DMG。Linux 执行 `python tools/package_linux.py --qt /absolute/path/.deps/qt` 生成 AppImage 和目录压缩包。缺少 esptool、固件或校验不符时发行检查失败。

## 自动构建

`.github/workflows/build.yml` 在 Windows、macOS ARM64、Ubuntu 22.04 原生构建、测试和打包。Qt SDK 与烧录工具来自锁定的官方发行包。Linux 配置 Xvfb 的 X11 和 Weston 的 Wayland 启动检查，产物再在 Ubuntu 24.04 和 Debian 12 的无开发环境容器启动。普通分支推送与 PR 上传 Actions 产物；推送与项目版本相符的 `v*` 标签（例如 `v1.0.2`）时，在三平台构建、包内固件与 esptool 检查、显示模拟以及 Linux 两个容器检查全部通过后，自动创建正式 GitHub Release。四个产物必须齐全，附 `SHA256SUMS.txt`；任何检查失败都不会发布部分平台的 Release。

正式签名不作为首版条件。macOS 发布方可在 CPack 前使用 `codesign --deep --force --options runtime --sign "$IDENTITY" dist/stage/AutoSplatoon.app`，随后以配置好的 `xcrun notarytool submit ... --keychain-profile ... --wait` 提交 DMG 并 stapler；Windows 发布方可用 signtool 对可执行文件签名后再打 ZIP。证书、账号及密码应只来自 CI secrets。未经签名的应用可能需要用户在系统安全设置中允许启动。

## 离线与许可证

固件既作为 Qt 资源编入程序，也以 `firmware/PRO-UART0.bin` 随包提供。运行时先核对 SHA-256 再写入临时文件，调用包内 esptool。Qt 采用动态链接，OpenCV 采用静态链接。随包提供 GPL 许可证、第三方声明和固件来源；发布方应同时提供与二进制对应的项目源码、依赖版本和构建说明。

## 发布流程

1. 更新 CMakeLists.txt 的项目版本与 main.cpp 的应用版本，编写 `docs/release-notes/v<版本>.md`。
2. 提交源码、内置固件、字体、依赖锁和文档；禁止提交 build、dist 或 .deps。
3. 推送提交，检查 Actions；创建并推送同版本标签：`git tag -a v1.0.2 -m "AutoSplatoon v1.0.2"`、`git push origin v1.0.2`。
4. 查看 Build and package 工作流。发布任务使用 GitHub 自带 GITHUB_TOKEN 的 contents: write 权限，无需额外发布密钥。仅在完整检查通过后发布，下载文件由 GitHub 原生 runner 构建。
5. 下载 Release 产物，按 SHA256SUMS.txt 校验；保持标签不可变。修复已发布版本时提升版本并创建新标签，不覆盖旧版本。

工作流支持选择已有版本标签手动运行，或对失败任务重新运行。GitHub CLI 按官方流程先创建草稿、上传全部资源再公开，避免上传过程中暴露不完整发行包。发布失败后先检查是否存在未公开草稿，在确认资产完整前不要手动公开。
