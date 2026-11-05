# MitamaOS site

The public site for MitamaOS, published at **https://xorbret.github.io/MitamaOS/**.

The `firmware` GitHub Actions workflow builds the firmware on every push to
`main` (changes under `mitamaos/firmware/**` or `docs/**`), copies this folder
next to the built binaries in `firmware/`, and deploys both to GitHub Pages.
Pages must be set to **Settings → Pages → Source: GitHub Actions**.

## Pages

| Page | What it is |
|------|------------|
| `index.html` | Home: the Mitama, the apps, the Compendium, Shugyo and Harae, install routes |
| `flash.html` | Web flasher (desktop Chrome/Edge), plus the SD card and esptool routes |
| `harae.html` | Harae quick how-to-play |
| `harae/` | Harae design: hub, the four factions, card gallery (`ofuda.js` holds the cards) |
| `mitama.html`, `ui.html`, `icons.html`, `type-scale.html`, `sound.html`, `logo.html` | Early concept mockups, kept for the record |

## Published by CI (not in this folder)

- `firmware/mitamaos-cardputer-adv-merged.bin`: full image for Launcher > SD or esptool at `0x0`
- `firmware/bootloader.bin`, `partitions.bin`, `boot_app0.bin`, `firmware.bin`: parts the web flasher writes (see `manifest-mitamaos-adv.json`)
- `firmware/version.txt`: short commit hash of the build. Molt on the device reads it to find updates, and the home page and flasher show it.

The same binaries are attached to the `m0-test` release.

## Keeping the home page current

The app list, Compendium races and ki ranks on `index.html` are plain arrays
in its script (`APPS`, `RACES`, `RANKS`). The app sigils are ported from
`mitamaos/firmware/src/ui/mitama_ui.h` (`MUI::sigil`) into `SIGILS`; when a
sigil changes in the firmware, change it there too.

The full written design lives in [`../mitamaos/outline.md`](../mitamaos/outline.md).
