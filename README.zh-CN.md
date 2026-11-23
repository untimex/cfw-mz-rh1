**由衷感谢 [Sir68k](https://github.com/Sir68k)！正是你的开创性工作，让索尼 MZ-RH1 这台经典的 Hi-MD 随身听在多年后仍能迎来新的可能。你不仅让自定义固件成为现实，还公开了源码、刷写工具和恢复方法，让后来者也能接着探索。这个分支里的显示和线控改进，是我站在巨人的肩膀上做的一点小小优化。谢谢你为 MZ-RH1 和整个 MiniDisc 社区打开了这扇门！**

[English](README.md)

# Sony MZ-RH1 自定义固件 1.3r11

1.3r11 是基于 [Sir68k/cfw-mz-rh1](https://github.com/Sir68k/cfw-mz-rh1) 的社区修改版。此版本在 1.2r5 基础上为 RM-MC35ELK和RM-MC40ELK 增加了一系列汉字显示优化功能，以及机身 OLED 的汉字处理设置。

## RM-MC35ELK 汉字显示

RM-MC35ELK 的硬件决定了它只能显示Shift-JIS 字符。曾经想在这个线控上完整显示中文，必须在写入歌曲时就把Unicode中文转换成Shift-JIS字符，非常麻烦。因此在这版固件里，内置了针对35ELK的字符转换功能。
当连接 RM-MC35ELK 并使用 Hi-MD 模式时，固件会自动尝试将 Unicode 曲名等文字转换为线控可以显示的 Shift-JIS 字符。遇到 Shift-JIS 缺少的简体汉字时，优先使用字表中可编码的对应字。转换只作用于线控显示，碟片上的文字标签保持原样。

注意此功能仅在Hi-MD模式下才能实现。因为传统MD的曲名仅支持Shift-JIS字符，Unicode中文根本无法写入曲名中。

| 原曲名 | 在线控上的显示 | 说明 |
| --- | --- | --- |
| 《阴天》 | 《陰天》 | 使用 Shift-JIS 可显示的对应繁体字，已在 RH1上实机验证。 |
| 《你说》 | 《祢説》 | 「祢」取字形相近，「説」采用日文汉字写法。 |
| `A日本汉语B` | `Ａ日本漢語Ｂ` | 转换后的混排文字使用双字节显示，拉丁字母与 ASCII 标点以全角显示。 |

Shift-JIS 字库覆盖范围有限。没有可用对应字的字符以替代符号显示。此转换由固件根据线控型号自动启用。

## RM-MC40ELK 线控设置

| `Option` 菜单项 | 功能 |
| --- | --- |
| `Backlight` | 自动熄灯可选 `4 sec`、`8 sec` 或 `16 sec`，另有 `On` 和 `Off`。秒数为标称值。 |
| `Jog Click` | 选择 `Backlight` 时，在自动背光档下，背光熄灭后第一次按下滚轮中央只点亮背光。亮屏时按下会执行原有功能。 |
| `Display` | 自定义 Hi-MD 的三行播放信息。 |
| `Group Skip` | 选择 `Long Press` 后，播放或停播时长按上一首或下一首可向对应方向跳组。碟片没有分组时，每次跳过十曲。 |

`Display` 提供 `Default`、`Title+Artist/Album`、`Info+Title+Artist`、`Info+Title+Album` 和 `Title+Artist+Album`。`Title+Artist/Album` 的第三行轮流显示艺术家和专辑，长文字会滚动到末尾后再切换。自定义布局的各行独立滚动，超出五个汉字或十个半角英文字符的显示宽度时才开始滚动。标准 MD 模式使用原厂布局，线控的 `DISPLAY` 键仍可切换原厂信息页。

## 机身 OLED

在 `CFW Settings → [ChineseCharacter]` 中，可选择 `Off`、`Pinyin`、`Original` 或 `Skip`，分别对应现有显示、拼音、遇到汉字时使用原厂布局和跳过汉字。这项设置只作用于机身 OLED。

## 刷写与回退

1. 使用平时的 RH1 刷写流程，先备份本机固件。
2. 解压发行包，选择 `RH1-Remote-1.3r11-35ELK-Character-Aliases.bin`。镜像为 1,044,480 字节（`0xFF000`）。
3. 等刷写工具完成写入和校验，并提示可以断开后，拔掉 USB，取出电池再重新装回。
4. 接上 35ELK，播放一首含「你」或「说」的 Hi-MD 曲目，确认替换后的显示。

[下载 1.3r11](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r11) · [刷写工具](https://sir68k.re/rh1/fw/)

r11 镜像的 SHA-256 为：

```text
8153251ddfb8853ce6f2e0ed49642390140e38dc8fe56f3af08f6fd07a9581a7
```


## 从源码构建

使用 [1.3r5 源码](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r5)、Arm GNU Toolchain 14.3.Rel1 和 Python 3.11。将发行包内的三份补丁放到源码目录，在该目录执行：

```sh
git apply 1.3r5-to-1.3r8-35ELK-Traditional.patch
git apply 1.3r8-to-1.3r10-35ELK-Wide-Test.patch
git apply 1.3r10-to-1.3r11-35ELK-Character-Aliases.patch
mkdir -p build
make -C src/extension-code
python3 tools/firmware-patcher/patcher.py src/patches.toml
```

输出文件为 `build/RH1-Remote-1.3r11-35ELK-Character-Aliases.bin`。发行包的 `SHA256SUMS.txt` 列出了 r11 镜像和 r10 回退镜像的校验值。

## 致谢与许可

感谢 Sir68k 开发 MZ-RH1 自定义固件和刷写工具。35ELK 的简繁转换字表使用 OpenCC 1.1.9 的 `STCharacters.txt` 生成，OpenCC 许可随发行包提供。机身 OLED 的拼音字表基于 ICU `Han-Latin` 数据。
