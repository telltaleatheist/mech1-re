# The setup screen's look

MechWarrior 1 Rebuild's badge on re-setup-kit's screens
(`src/SDL/SDLSetup.cpp`, `theme()`). The look and its fonts are the kit's own,
the same in every game. Nothing here is the original game's.

| file | what | source |
| --- | --- | --- |
| `badge.png` | the badge beside the title, 320x320 with alpha | the round medallion the BattleTech launcher shows for this game, mw3-re's `assets/launcher/badge_mw1o.png` (the same file) |

The badge is this project's own generated art and holds nothing from the game: Qwen-Image 2.1, 768x768, 40 steps,
seed 62, cut out for the launcher (the black surround made transparent, the picture cropped square at 320x320). The
prompt: "A round military medallion badge: a BattleMech walking in side view drawn as a chunky low-resolution 1980s
EGA computer game sprite, big square pixels, only a few flat colours, a humanoid war robot with a squat torso, stubby
arms with gun barrels and thick legs, the pixel art set into embossed brushed steel with green phosphor enamel inlays
like an old computer monitor, inside a thick circular brushed steel rim with small rivets around it, symmetrical,
front view, centred on a pure black background, no text, no letters." The program's icon
(`tools/package/mechwarrior.png`) is a different picture.
