[简体中文](README.zh-CN.md)

# Sony MZ-RH1 Custom Firmware 1.3r0

This community build for the Sony MZ-RH1 is based on [Sir68k/cfw-mz-rh1](https://github.com/Sir68k/cfw-mz-rh1). It adds backlight, display layout, and group navigation settings for the **RM-MC40ELK three-line remote** and improves the handling of track information containing Han characters on the recorder's OLED displays.

## RM-MC40ELK remote

These settings are in the remote's `Option` menu. The firmware enables them when it detects a connected RM-MC40ELK in three-line display mode. Other remote models use the stock menus and button behavior.

### Backlight

The stock backlight often went out before I had finished reading the display. The existing `Backlight` menu now offers **`4 sec`, `8 sec`, and `16 sec`** for automatic shutoff, along with `On` and `Off`. The durations are nominal values based on the firmware's timer.

### Display

The RM-MC40ELK has a three-line display, but the first line of the stock Hi-MD playback screen is left blank. These layouts let you use all three lines to show more information.

| Setting | First line | Second line | Third line |
| --- | --- | --- | --- |
| `Default` | Stock layout | Stock layout | Stock layout |
| `Info+Title+Artist` | Track number and playback time | Track title | Artist |
| `Info+Title+Album` | Track number and playback time | Track title | Album |
| `Title+Artist+Album` | Track title | Artist | Album |

Artist and album are separate track fields in Hi-MD mode. When a disc is used in standard MD mode, the `Display` option is hidden and the playback screen uses the stock layout. The saved custom layout returns when you switch back to Hi-MD. The remote's `DISPLAY` button still cycles through the stock information pages.

### Jog Click

On the stock firmware, pressing a button to light the backlight also activates that button. I often just want to read the track information. With `Jog Click` set to `Backlight`, pressing the center of the jog dial while the light is off only turns on the light. Pressing it again while the light is on performs its stock action.

- **`Default`** keeps the stock center-click behavior.
- **`Backlight`** works with the `4 sec`, `8 sec`, and `16 sec` automatic backlight settings.

### Group Skip

I was surprised that the RM-MC40ELK had no dedicated group navigation controls, so I mapped them to a long press of the previous and next track buttons.

| Setting | Previous and next buttons during playback or while stopped |
| --- | --- |
| `Off` (default) | Use the stock button behavior. |
| `Long Press` | A short press performs the stock single-track action on release. A long press skips to the previous or next group. On discs without groups, it uses the stock ten-track skip. |

Each long press triggers one jump. Track boundaries and play modes follow the stock firmware, as do button actions in menus and editing screens. [Sony MZ-RH1 Operating Instructions, page 40](https://www.minidisc.org/manuals/sony/Sony_MZ-RH1_user_manual.pdf#page=40).

## Recorder OLED displays

Sir68k's 1.2r5 CFW adds flexible track information layouts to the recorder's OLED displays. The displays cannot render Han characters, which can make track information containing those characters unreadable. This build adds a setting to handle them.

On the recorder, open `CFW Settings → [ChineseCharacter]`. The setting applies to the track, group, disc, artist, and album text shown by the current `TrackInfoStyle` layout, depending on the disc format and the selected layout. It affects only the recorder's OLED displays.

| Setting | Display behavior |
| --- | --- |
| `Off` (default) | Use the existing `TrackInfoStyle` behavior. |
| `Pinyin` | Convert Han characters with a known reading to tone-free pinyin. Characters without a matching reading appear as `?`. |
| `Original` | Temporarily use the stock `1.0A0` layout when the current track's displayed fields contain Han characters. The chosen `TrackInfoStyle` layout returns when you move to a track whose displayed fields have no Han characters. |
| `Skip` | Omit Han characters and retain the remaining text the OLED can display. |

The firmware identifies individual Han characters, so Japanese kanji can also trigger these modes. Pinyin uses Mandarin readings, and characters with multiple readings may be rendered incorrectly in context. When the OLED goes dark, track information conversion pauses and redundant clearing and refreshes are reduced. Updates resume when the display lights again.

## Installation and verification

Get the `RH1-Remote-1.3r0-Group-Skip.bin` image from the [1.3r0 release](https://github.com/untimex/cfw-mz-rh1/releases/tag/rh1-1.3r0). The image is `0xFF000` bytes. Back up your recorder's firmware before flashing with the upstream [MZ-RH1 WebUSB flasher](https://sir68k.re/rh1/fw/). Wait until writing and verification finish and the tool says it is safe to disconnect. Unplug the USB cable, remove the battery, and reinstall it to boot the firmware. The documented forced-recovery procedure requires access to the mainboard. Recovery without opening the recorder has yet to be verified. [Upstream flashing and recovery notes](https://sir68k.re/posts/rh1-firmware-available/#recovery).

The 1.3r0 firmware was flashed and tested on one MZ-RH1.

| File | Purpose |
| --- | --- |
| `RH1-Remote-1.3r0-Group-Skip.bin` | Flashable image |
| `RH1-Remote-1.3r0-Group-Skip.patch` | Complete source patch against the upstream base commit |
| `SHA256SUMS.txt` | Release file checksums |

The firmware image SHA-256 is `28920a6cd722d90be57ad5f6bd4c39df19f0d78c086701076cf6723b2bb99471`.

## Building from source

Use Arm GNU Toolchain 14.3.Rel1 and Python 3.11. The [upstream base commit](https://github.com/Sir68k/cfw-mz-rh1/tree/9fb8d45c492eedc45044f8de7e50f1635a7a020a) is `9fb8d45c492eedc45044f8de7e50f1635a7a020a`. In an upstream checkout at that commit, place the release patch in the repository root and apply it with the first command below. In this fork, start with `mkdir -p build`.

```sh
git apply RH1-Remote-1.3r0-Group-Skip.patch
mkdir -p build
make -C src/extension-code
python3 tools/firmware-patcher/patcher.py src/patches.toml
```

The output is `build/RH1-Remote-1.3r0-Group-Skip.bin`. An independent build produced a byte-for-byte match with the release image.

## Credits and license

Thank you to Sir68k (Jim B.) for creating the MZ-RH1 custom firmware and flasher and for his contributions to the RH1 community. The pinyin lookup tables are generated from ICU's `Han-Latin` transliteration data. The upstream MIT license and the ICU license are included with the release.
