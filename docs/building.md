# 构建与发布

构建使用 C++17、CMake、Ninja 和 Python 3.15。下载资产的版本及 SHA-256 见 [依赖锁文件](../dependencies.lock.json)，Ninja 版本见 [构建依赖清单](../tools/requirements-build.txt)。

## 本地构建

安装 Python 3.15.2，然后执行以下 Windows x64 示例。`install_qt.py` 下载并校验官方 CMake、Qt、匹配的 MinGW 和 7-Zip，并输出 `CMAKE_BIN` 和 `QT_ROOT`。

```powershell
python -m venv .deps/venv
.deps/venv/Scripts/python -m pip install -r tools/requirements-build.txt
.deps/venv/Scripts/python tools/install_qt.py --platform windows-amd64
$env:PATH = "$PWD/.deps/cmake/4.4.5/bin;$PWD/.deps/venv/Scripts;$PWD/.deps/compiler/mingw64/bin;$PWD/.deps/qt/bin;$env:PATH"
python tools/fetch_assets.py --platform windows-amd64 --output build/release/assets
cmake --preset release -DCMAKE_PREFIX_PATH="$PWD/.deps/qt" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build --preset release --parallel 3
windeployqt --release --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler --skip-plugin-types generic,networkinformation,tls --include-plugins qoffscreen build/release/autosplatoon_tests.exe
New-Item -ItemType Directory -Force build/test-temp | Out-Null
$env:TEMP = (Resolve-Path build/test-temp).Path
$env:TMP = $env:TEMP
ctest --preset release --no-tests=error
cpack --config build/release/CPackConfig.cmake -B dist
```

macOS 使用 `macos-arm64` 和 Xcode/Apple Clang，配置时指定 `-DCMAKE_OSX_ARCHITECTURES=arm64`，最低系统版本为 14.4。Linux 使用 `linux-amd64` 和 GCC，构建基线为 Ubuntu 22.04。两者将 `CMAKE_BIN`、虚拟环境的 `bin` 和 Qt 的 `bin` 加入 PATH，省略 Windows 编译器参数和 windeployqt。Linux 构建所需系统库清单见 [工作流](../.github/workflows/build.yml)。执行 `python tools/package_linux.py --qt "$QT_ROOT"` 生成 AppImage 和目录包。

OpenCV 从锁定源码静态构建 core、imgproc、photo 及其必需的 geometry/flann 依赖。图片格式由 Qt 插件提供。Qt 动态链接，发行包携带各组件许可证及 Qt SBOM。MinGW 使用 Qt SDK 对应的 15.1.0 工具链，详见 [Qt 6.12 工具版本](https://wiki.qt.io/Qt_6.12_Tools_and_Versions)。

## 验证

按 [验证方法](validation.md) 检查最终归档解压或挂载后的文件。Linux 使用官方 linuxdeploy Qt 插件部署 offscreen、Wayland 与 SVG。

## 发布

1. 更新 CMake 项目版本，编写该版本的 `docs/release-notes/v<版本>.md`；应用版本由构建配置传入。
2. 提交改动，推送与项目版本相符的 `v<版本>` 标签。各平台通过 CI 后，工作流发布 Windows ZIP、macOS DMG、Linux AppImage、目录包及 `SHA256SUMS.txt`。
3. 下载校验清单，对照各资产 SHA-256。已发布标签应保持不变。

发布任务使用 GitHub Actions 自带令牌。当前发行包没有正式代码签名或 macOS 公证。
