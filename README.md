[简体中文](README.zh-CN.md)

# Sony MZ-RH1 Custom Firmware 1.3r11

This community build is based on Sir68k's custom firmware 1.2r5. It adds Han-character display improvements for the RM-MC35ELK and RM-MC40ELK remotes, along with Chinese character handling for the recorder's OLED displays. [Sir68k/cfw-mz-rh1](https://github.com/Sir68k/cfw-mz-rh1)

## RM-MC35ELK character display

The RM-MC35ELK displays Shift-JIS characters. Previously, displaying Chinese text on this remote meant converting Unicode titles to Shift-JIS before transferring the tracks. In Hi-MD mode, the firmware now converts Unicode titles and other text for the remote automatically. When a simplified Chinese character is unavailable in Shift-JIS, the firmware looks up an encodable substitute. The text stored on the disc stays intact.

This feature requires Hi-MD mode. Standard MD track titles use the original title encodings, so the same Unicode Chinese titles cannot be stored in that mode.

| Original title | Remote display | Notes |
| --- | --- | --- |
| 阴天 | 陰天 | Uses the Shift-JIS traditional character. Verified on a physical RH1. |
| 你说 | 祢説 | 祢 is a visually similar substitute, and 説 is the Japanese character form. |
| `A日本汉语B` | `Ａ日本漢語Ｂ` | Converted mixed text uses double-byte display, with full-width Latin letters and ASCII punctuation. |

Characters without an encodable match appear as placeholders. The firmware enables the conversion automatically when it detects an RM-MC35ELK.

## RM-MC40ELK remote settings

| `Option` menu item | Function |
| --- | --- |
| `Backlight` | Choose `4 sec`, `8 sec`, or `16 sec` for automatic shutoff, or use `On` or `Off`. The time values are nominal. |
| `Jog Click` | With `Backlight` selected and an automatic shutoff interval active, pressing the center of the jog dial while the light is off turns on the light. Pressing it while the light is on performs its original action. |
| `Display` | Customize the three-line Hi-MD playback display. |
| `Group Skip` | With `Long Press` selected, holding Previous or Next during playback or while stopped skips to the adjacent group. On a disc without groups, it jumps ten tracks in that direction. |

`Display` offers `Default`, `Title+Artist/Album`, `Info+Title+Artist`, `Info+Title+Album`, and `Title+Artist+Album`. In `Title+Artist/Album`, the third line alternates between artist and album. Long names finish scrolling before the line switches. Each line in a custom layout scrolls independently when its text exceeds the width of five Han characters or ten half-width Latin characters. Standard MD mode uses the stock layout. The remote's `DISPLAY` button still cycles through the stock information pages.

## Recorder OLED displays

Open `CFW Settings → [ChineseCharacter]` on the recorder. `Off` uses the existing display behavior. `Pinyin` displays pinyin, `Original` uses the stock layout when displayed text contains Han characters, and `Skip` omits those characters. This setting applies only to the recorder's OLED displays.

## Flashing and rollback

1. Back up your recorder's firmware using your usual RH1 flashing procedure.
2. Extract the release package and select `RH1-Remote-1.3r11-35ELK-Character-Aliases.bin`. The image is 1,044,480 bytes (`0xFF000`).
3. Wait for the flasher to finish writing and verifying the image and tell you it is safe to disconnect. Unplug USB, remove the battery, and reinstall it.
4. Connect an RM-MC35ELK and play a Hi-MD track containing 你 or 说 to check the displayed substitute.

[Download 1.3r11](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r11) · [MZ-RH1 WebUSB flasher](https://sir68k.re/rh1/fw/)

### Firmware SHA-256

```text
8153251ddfb8853ce6f2e0ed49642390140e38dc8fe56f3af08f6fd07a9581a7
```

## Building from source

Use Arm GNU Toolchain 14.3.Rel1 and Python 3.11. Starting from the 1.3r5 source, place the three patches from the release package in the repository root and run the following commands. [1.3r5 source](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r5)

```sh
git apply 1.3r5-to-1.3r8-35ELK-Traditional.patch
git apply 1.3r8-to-1.3r10-35ELK-Wide-Test.patch
git apply 1.3r10-to-1.3r11-35ELK-Character-Aliases.patch
mkdir -p build
make -C src/extension-code
python3 tools/firmware-patcher/patcher.py src/patches.toml
```

The output is `build/RH1-Remote-1.3r11-35ELK-Character-Aliases.bin`. The release package's `SHA256SUMS.txt` contains checksums for the r11 image and the r10 rollback image.

## Credits and licensing

Thanks to Sir68k for developing the MZ-RH1 custom firmware and flasher. The RM-MC35ELK conversion table uses OpenCC 1.1.9's `STCharacters.txt`, and the OpenCC license is included with the release package. The recorder's pinyin table uses ICU `Han-Latin` data.
