# AutoSplatoon 的内置示例与处理效果

在空白工作台点击示例卡，或从“导入图片”的展开菜单选择“示例素材”。这三组示例在软件内可以离线处理、调整和导出。

| 示例 | 彩色源图 | 清晰线稿对照 | 四预设对照 | 320×120 原始输出 |
| --- | --- | --- | --- | --- |
| 鱿鱼集合 | [源图](examples/squid-party-source.png) | [前后对比](examples/zh-CN/squid-party-before-after.png) | [预设对比](examples/zh-CN/squid-party-presets.png) | [线稿](examples/squid-party-line-art.png) · [素描](examples/squid-party-pencil.png) · [阈值](examples/squid-party-threshold.png) · [抖动](examples/squid-party-dither.png) |
| 墨迹派对 | [源图](examples/ink-party-source.png) | [前后对比](examples/zh-CN/ink-party-before-after.png) | [预设对比](examples/zh-CN/ink-party-presets.png) | [线稿](examples/ink-party-line-art.png) · [素描](examples/ink-party-pencil.png) · [阈值](examples/ink-party-threshold.png) · [抖动](examples/ink-party-dither.png) |
| 手柄线稿 | [源图](examples/controller-lines-source.png) | [前后对比](examples/zh-CN/controller-lines-before-after.png) | [预设对比](examples/zh-CN/controller-lines-presets.png) | [线稿](examples/controller-lines-line-art.png) · [素描](examples/controller-lines-pencil.png) · [阈值](examples/controller-lines-threshold.png) · [抖动](examples/controller-lines-dither.png) |

工作台截图可查看[示例页](examples/zh-CN/sample-gallery.png)、[浅色主题](examples/zh-CN/workbench-light.png)、[深色主题](examples/zh-CN/workbench-dark.png)和[手动控制](examples/zh-CN/manual-control.png)。

## 示例采用同一组图像参数

| 参数 | 值 |
| --- | --- |
| 细节 | 50 |
| 去噪 | 1 |
| 线条粗细 | 1 |
| 阈值 | 160 |
| 反色 | 关闭 |

原始输出由工作台载入示例并逐一切换预设后保存。图像尺寸和黑白像素已核对，效果对比图以整数倍放大。源图和输出文件的校验值见[生成记录](examples/examples.json)。

## 生成方式与素材署名

使用 `cmake --build --preset release --target autosplatoon_examples` 构建示例生成工具。将 Qt 运行库加入运行环境后，设置 `QT_QPA_PLATFORM=offscreen` 并运行 `build/release/autosplatoon_examples docs/images/examples zh_CN`。生成英文图片时将参数改为 `en`。界面截图和带文字的对比图分别写入 `zh-CN/` 和 `en/`，源图和无文字输出共用。Windows 可执行文件后缀为 `.exe`。

新界面使用 Mzyy94 的 Slidoon 鱿鱼和墨迹、Openclipart 的 Ink Splash、Kenney 纹理、Zacksly 的 Switch 图标以及 Delapouite 的滚筒图标。逐文件来源、许可、改动与校验值见[美术素材清单](../../licenses/ARTWORK.json)和[第三方声明（英文）](../../licenses/THIRD_PARTY.md)。

同目录原有 `squid-*.png` 为历史示例，使用 [Keebo 的 Splatoon Cursors](https://www.rw-designer.com/cursor-set/splatoon)，发布条款为 Public Domain。原始文件及作者声明位于 `squid-originals/`。Splatoon 角色版权归 Nintendo 所有。
