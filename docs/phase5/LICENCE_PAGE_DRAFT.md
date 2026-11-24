# Licence page, draft

A draft for the project owner to settle: what each package ships, and under what licence. It changes nothing.
Every package carries the full texts in its `licenses` folder.

**The rights in the original.** MechWarrior (1989), BattleTech, BattleMech and the original's art, music, sound, text
and data belong to their rights holders. Nothing of the game is in the packages. You need your own copy.

**The program.** The program is under the MIT licence. It is built on Elthial's reconstruction, also MIT
(copyright Elthial, 2026), whose notices are kept.

## What the packages ship

| Item | Windows | Mac | Linux (tar) | AppImage | Licence |
| --- | --- | --- | --- | --- | --- |
| The program | yes | yes | yes | yes | MIT, as above |
| SDL3 | yes | yes | yes | yes | zlib |
| hidapi, compiled into SDL3 | yes | yes | yes | yes | BSD style (the BSD option of its three) |
| libarchive 3.7.9 | yes | yes | yes | yes | BSD 2-Clause |
| zlib 1.3.1 | yes | yes | yes | yes | zlib |
| xz 5.8.1, liblzma | yes | yes | yes | yes | 0BSD |
| re-setup-kit | yes | yes | yes | yes | MIT |
| stb_truetype, stb_image, in the kit | yes | yes | yes | yes | MIT or public domain |
| miniz's inflate, in the kit | yes | yes | yes | yes | MIT |
| TinySoundFont | yes | yes | yes | yes | MIT |
| FluidR3 GM 3.1, a subset cut at build time | yes | yes | yes | yes | MIT (copyright Frank Wen) |
| Saira Condensed Bold, the setup screen's face | yes | yes | yes | yes | SIL OFL 1.1 (reserved name "Saira") |
| The badge, the program's icon | yes | yes | yes | yes | the MW1 remake's own, used with leave; no trademark rights |
| Microsoft Visual C++ runtime: three DLLs | yes | no | no | no | Microsoft redistributable, as its licence allows |
| Inno Setup's installer code in Setup.exe | yes | no | no | no | Inno Setup's licence, which allows distributing installers it makes |
| AppImage runtime (type 2, release 20251108) | no | no | no | yes | MIT |
| libfuse 3.15.0, in that runtime | no | no | no | yes | LGPL 2.1 |
| squashfuse 0.5.2, in that runtime | no | no | no | yes | BSD 2-Clause |
| Zstandard, in that runtime | no | no | no | yes | BSD 3-Clause |
| zlib, mimalloc, musl, in that runtime | no | no | no | yes | zlib; MIT; MIT |

## Where the texts are

- All packages: `licenses/` (project, SDL, hidapi, kit, stb, miniz, TinySoundFont, FluidR3's COPYING and README,
  libarchive, xz, zlib) with the NOTICE and the third-party notices. The font's licence is in `data/setup`.
- The AppImage: also `licenses/appimage-runtime`, with the notice, every licence text and where the source is.
- Windows: the third-party notices name the Microsoft files.

## For the owner to settle

- The wording above, and whether this page ships in the packages, on the release page, or both.
- Inno Setup is listed here only for the compiler's own code in Setup.exe; the packages' notices do not list it today.
