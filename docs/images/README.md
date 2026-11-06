# Built-in samples and preset results

Select a sample card in the empty workbench or open **Samples** from the **Import Image** dropdown. All three samples can be edited, processed, and exported offline.

| Sample | Color source | Line Art comparison | All four presets | Original 320 × 120 output |
| --- | --- | --- | --- | --- |
| Squid Party | [Source](examples/squid-party-source.png) | [Comparison](examples/en/squid-party-before-after.png) | [Preset grid](examples/en/squid-party-presets.png) | [Line Art](examples/squid-party-line-art.png) · [Pencil Sketch](examples/squid-party-pencil.png) · [Threshold](examples/squid-party-threshold.png) · [Dither](examples/squid-party-dither.png) |
| Ink Party | [Source](examples/ink-party-source.png) | [Comparison](examples/en/ink-party-before-after.png) | [Preset grid](examples/en/ink-party-presets.png) | [Line Art](examples/ink-party-line-art.png) · [Pencil Sketch](examples/ink-party-pencil.png) · [Threshold](examples/ink-party-threshold.png) · [Dither](examples/ink-party-dither.png) |
| Controller Lines | [Source](examples/controller-lines-source.png) | [Comparison](examples/en/controller-lines-before-after.png) | [Preset grid](examples/en/controller-lines-presets.png) | [Line Art](examples/controller-lines-line-art.png) · [Pencil Sketch](examples/controller-lines-pencil.png) · [Threshold](examples/controller-lines-threshold.png) · [Dither](examples/controller-lines-dither.png) |

[Sample gallery](examples/en/sample-gallery.png) · [Light workbench](examples/en/workbench-light.png) · [Dark workbench](examples/en/workbench-dark.png) · [Small window](examples/en/workbench-small.png) · [Manual Controls](examples/en/manual-control.png) · [Flashing](examples/en/flash-dialog.png) · [Getting Started](examples/en/guide-dialog.png)

<a name="示例采用同一组图像参数"></a>

## Image settings

| Setting | Value |
| --- | --- |
| Detail | 50 |
| Denoise | 1 |
| Line width | 1 |
| Threshold | 160 |
| Invert black and white | Off |

The workbench loaded each sample and saved the result of each preset. The original outputs were checked for dimensions and binary pixels. The comparison images enlarge them by an integer factor. See the [generation manifest](examples/examples.json) for source and output checksums.

<a name="生成方式与素材署名"></a>

## Generate examples and credit artwork

Build the tool with `cmake --build --preset release --target autosplatoon_examples`. Make the Qt runtime available, set `QT_QPA_PLATFORM=offscreen`, and run `build/release/autosplatoon_examples docs/images/examples en`. Use `zh_CN` for Simplified Chinese. The tool writes screenshots and captioned comparisons to `en/` and `zh-CN/` respectively and shares the source images and unlabelled outputs. On Windows, the executable has an `.exe` suffix.

The interface uses Mzyy94's Slidoon squid and ink artwork, Openclipart's Ink Splash, Kenney textures, Zacksly's Switch icons, and Delapouite's paint roller. See the [artwork manifest](../../licenses/ARTWORK.json) and [third-party notices](../../licenses/THIRD_PARTY.md) for per-file attribution, licenses, changes, and checksums.

The original `squid-*.png` files in this directory are historical examples using [Keebo's Splatoon Cursors](https://www.rw-designer.com/cursor-set/splatoon), released into the public domain. The original files and author's notice are in `squid-originals/`. Splatoon characters belong to Nintendo.
