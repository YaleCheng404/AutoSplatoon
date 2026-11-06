# Build and release

The project uses C++17, CMake, Ninja, and Python 3.15. See the [dependency lockfile](../dependencies.lock.json) for versions and SHA-256 hashes and the [build requirements](../tools/requirements-build.txt) for the Ninja version.

<a name="本地构建"></a>

## Local build

Install Python 3.15.2, then follow this Windows x64 example. The SDK installer downloads and verifies CMake, Qt, the matching MinGW compiler, and 7-Zip. It prints `CMAKE_BIN` and `QT_ROOT`.

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

On macOS, use `macos-arm64` with Xcode and Apple Clang. Configure with `-DCMAKE_OSX_ARCHITECTURES=arm64`. The minimum macOS version is 14.4. On Linux, use `linux-amd64` with GCC. The build baseline is Ubuntu 22.04. Add `CMAKE_BIN`, the virtual environment's `bin`, and Qt's `bin` to PATH. Omit the Windows compiler flags and the windeployqt step. See the [build workflow](../.github/workflows/build.yml) for Linux system libraries. Run `python tools/package_linux.py --qt "$QT_ROOT"` to build the AppImage and directory archive.

OpenCV is built statically from the pinned source, with core, imgproc, photo, and their required geometry and flann modules. Qt plugins provide image formats. Qt is dynamically linked and packages include the component notices and Qt SBOM. The Windows build uses the Qt SDK's matching MinGW 15.1.0 toolchain. See [Qt 6.12 tool versions](https://wiki.qt.io/Qt_6.12_Tools_and_Versions).

## Interface translations

Mark user-visible Simplified Chinese source strings with Qt's `tr()` or `QCoreApplication::translate()`. Edit [the English catalog](../translations/autosplatoon_en.ts), then run `python tools/check_translations.py --extract build/qa/translation-source.ts` to check coverage and placeholders. The command also writes a source-only catalog for review. With a Qt SDK that includes QML, you can run the `update_translations` target to update source entries. Ordinary builds use Qt Linguist's `lrelease` to compile the catalog. The pinned Linux and macOS SDKs include Qt Declarative so that `lrelease` can load Qt QML. The executable embeds the English catalog and the Chinese Qt base catalog.

<a name="验证"></a>

## Validation

Follow the [validation instructions](validation.md) with the extracted or mounted packages. Linux uses the official linuxdeploy Qt plugin to deploy the offscreen, Wayland, and SVG plugins.

<a name="发布"></a>

## Release

1. Update the CMake project version and write the English notes in `docs/release-notes/v<version>.md`. The build passes the project version to the app.
2. Commit the changes and push a `v<version>` tag that matches the project version. After CI passes, the workflow publishes a Windows ZIP, macOS DMG, Linux AppImage and directory archive, and `SHA256SUMS.txt`.
3. Download the checksum list and verify each asset. Keep published tags intact.

The workflow uses the GitHub Actions token. Current packages lack formal code signing and macOS notarization.
