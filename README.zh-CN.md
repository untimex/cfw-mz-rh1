[English](README.md)

# Sony MZ-RH1 自定义固件 1.3r0

1.3r0 是基于 [Sir68k/cfw-mz-rh1](https://github.com/Sir68k/cfw-mz-rh1) 的社区修改版，面向 Sony MZ-RH1。主要改动是给 **RM-MC40ELK 三行线控**优化背光、显示布局和跳组设置，并改善机身 OLED 的汉字曲目信息显示。

## RM-MC40ELK 线控

下列设置位于线控的 `Option` 菜单。固件根据连接状态、线控标识和三行显示模式启用这些设置，其他线控沿用原厂菜单和按键逻辑。

### Backlight
原厂的线控背光亮起时间太短了，往往信息还没读完背光就熄灭了，因此增加了这个选项。

在原厂 `Backlight` 菜单中，自动熄灯可选 **`4 sec`、`8 sec`、`16 sec`**，另有 `On` 和 `Off`。秒数为按固件计时推算的标称值。

### Display

40ELk明明有三行显示，但是在Hi-MD模式下播放，线控屏幕第一行是空白或者group，而group应用场景不多，大部分情况下浪费了显示空间。因此设置了这个选项，可以完整利用40ELk的三行空间显示更多信息。

| 选项 | 第一行 | 第二行 | 第三行 |
| --- | --- | --- | --- |
| `Default` | 原厂布局 | 原厂布局 | 原厂布局 |
| `Info+Title+Artist` | 曲号和播放时间 | 曲名 | 艺术家 |
| `Info+Title+Album` | 曲号和播放时间 | 曲名 | 专辑 |
| `Title+Artist+Album` | 曲名 | 艺术家 | 专辑 |

艺术家和专辑行依赖 Hi-MD 曲目的相应字段。标准 MD 模式下按曲目保存的文字标签为曲名，因此会隐藏此选项，并恢复原厂显示布局。切回 Hi-MD 后，之前保存的布局会恢复。`DISPLAY` 键切换的其他原厂信息页保持可用。

### Jog Click
此设置为优化40ELK线控的点亮背光。原机设置无论按哪个按钮，都会在触发背光的时候触发按键功能，但是更多的时候会需要只点亮背光来读取歌曲信息。因此增加了这个选项，在背光熄灭时，点击Jog dial的确定键只是点亮背光。背光亮起时Jog dial的确定键恢复其原功能。

- **`Default`** 保留原厂中央滚轮按键行为。
- **`Backlight`** 适用于 `4 sec`、`8 sec` 或 `16 sec` 自动背光档。

### Group Skip

我惊奇的发现40ELK在原厂设置下竟然没有十曲跳过，或者Group功能。因此我把这个功能设定在了长按上一曲和下一曲上。

| 选项 | 播放或停播界面上的上一首和下一首 |
| --- | --- |
| `Off`（默认） | 使用原厂按键行为。 |
| `Long Press` | 短按在松手时执行原厂单曲操作。长按向对应方向跳组，碟片无分组时使用原厂十曲跳转。 |

每次长按触发一次。曲目边界和播放模式由原厂逻辑处理，菜单和编辑界面沿用原厂按键行为。[索尼 MZ-RH1 操作手册第 40 页](https://www.minidisc.org/manuals/sony/Sony_MZ-RH1_user_manual.pdf#page=40)。

## 机身 OLED

Sir68k的v1.2r5 CFW固件提供了非常好的Track Info Display Mode在机身的OLED上，可惜RH1的机身OLED显示不支持任何的汉字，导致如果有汉字信息的曲目在OLED上显示就会出现乱码。因此我加入了这个辅助设定。

在机身的 `CFW Settings → [ChineseCharacter]` 中选择汉字处理方式。适用范围为 `TrackInfoStyle` 当前布局显示的曲名、组名、碟名、艺术家和专辑等字段，具体取决于碟片格式和布局。该设置只作用于机身 OLED 的显示内容。

| 选项 | 显示方式 |
| --- | --- |
| `Off`（默认） | 使用现有的 `TrackInfoStyle` 显示方式。 |
| `Pinyin` | 将能够查表的汉字转为无声调拼音。无对应读音的字以 `?` 表示。 |
| `Original` | 当前曲目要显示的字段含汉字时，机身暂时使用原厂 `1.0A0` 的显示布局。切换到无汉字的曲目后，继续使用所选的 `TrackInfoStyle` 布局。 |
| `Skip` | 省略显示字段中的汉字，保留 OLED 能显示的其余内容。 |

固件按字符识别汉字，因此日文汉字也可能触发这些设置。拼音使用普通话字音，多音字读音可能和语境不同。机身 OLED 熄屏时暂停曲目信息转换，并减少重复的清屏和刷新操作，屏幕点亮后再更新，以节约电量。

## 安装与验证

从 [1.3r0 发布页](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r0) 获取 `RH1-Remote-1.3r0-Group-Skip.bin`，长度为 `0xFF000` 字节。使用上游 [MZ-RH1 WebUSB 刷写工具](https://sir68k.re/rh1/fw/) 前，请备份本机固件。等待写入和校验完成、工具提示可以断开后，断开 USB 并取出电池，重新装回电池以启动固件。已记录的强制恢复方式需要接触主板，免拆恢复尚无验证。[上游刷写与恢复说明](https://sir68k.re/posts/rh1-firmware-available/#recovery)。

在一台 MZ-RH1 上，1.3r0固件已经实机刷入并测试通过。

| 文件 | 用途 |
| --- | --- |
| `RH1-Remote-1.3r0-Group-Skip.bin` | 刷写镜像 |
| `RH1-Remote-1.3r0-Group-Skip.patch` | 相对于上游基线的完整源码补丁 |
| `SHA256SUMS.txt` | 发布文件的校验值 |

刷写镜像的 SHA-256 为 `28920a6cd722d90be57ad5f6bd4c39df19f0d78c086701076cf6723b2bb99471`。

## 从源码构建

使用 [上游基线提交](https://github.com/Sir68k/cfw-mz-rh1/tree/9fb8d45c492eedc45044f8de7e50f1635a7a020a)、Arm GNU Toolchain 14.3.Rel1 和 Python 3.11。在上游提交的源码目录执行以下命令，补丁文件需放在当前目录。若使用本 fork 的源码，从 `mkdir -p build` 开始执行。

```sh
git apply RH1-Remote-1.3r0-Group-Skip.patch
mkdir -p build
make -C src/extension-code
python3 tools/firmware-patcher/patcher.py src/patches.toml
```

生成的镜像位于 `build/RH1-Remote-1.3r0-Group-Skip.bin`。发布镜像已通过独立重编译的逐字节对比。

## 致谢与许可

感谢 Sir68k（Jim B.）开发 MZ-RH1 自定义固件和刷写工具，Sir68k对于RH1的贡献是无可替代的。拼音查找表由 ICU `Han-Latin` 转写数据生成。上游 MIT 许可和 ICU 许可随发行包提供。
