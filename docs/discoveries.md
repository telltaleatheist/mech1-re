# How MechWarrior (1989) really works

Things we found while rebuilding the 1989 DOS game, written for players and
programmers alike. Each one comes from the game's own programs. The address is
given so anyone can check: `MW_MAIN` is the campaign program and `BTECH` the
combat one, each with segment:offset in the image as it loads. "Likely" marks a
guess.

## Three programs, not one

- **The campaign and the battles are two separate programs.** `MW_MAIN.EXE`
  runs the company: planets, the bar, the Mech complex, contracts and travel.
  `BTECH.EXE` runs a battle in 3D. A small launcher, `MW.EXE`, starts one, waits
  for it to end, and starts the other. Every time you deploy, the campaign
  program quits and the combat program starts from nothing; when the battle
  ends, it quits and the campaign starts again with the results. In 1989 that
  was how a game this size fitted in 640 KB of memory.
- **The campaign checks that the launcher started it.** The launcher keeps an
  11-byte block in its data (`MW.EXE` 0806:07D4), and the campaign tests three
  of those bytes as it starts (`MW_MAIN` 081B:07D4, 07D8 and 07DE). Nothing ever
  reads the other eight. Likely this stopped players running the campaign
  program on its own.
- **Before every deployment the game looks for the combat program on disk.**
  On floppies, `BTECH.EXE` sat on a different disk from the campaign, so the
  game checked for it before each battle and asked for the disk if it wasn't
  there. The rebuild drops those checks and the disk prompts.

## The two releases

- **The 1990 update to the campaign program changed only the copy
  protection.** The 1989-11-03 and 1990-01-22 copies of `MW_MAIN.EXE` are the
  same size and differ in 11 bytes, all in the copy-protection code
  (081B:174A..1782). Everything else, the whole of the campaign's data
  included, is byte for byte the same.
- **The 1990 combat program is 266 bytes longer, and one fix shows.** In
  combat's two-line dialogs (such as the retreat question), the 1990 version
  waits for the key to be released, then clears the joystick buttons before
  going back to the cockpit (`BTECH` 0810:27F4 and 0810:29B2 in the 1990
  build). The 1989 version waits for the key to change instead. Likely the fix
  stops the key that answers a dialog from also counting in the cockpit. The
  two versions' battles run one tick apart in our recordings, so a recording
  made on one doesn't match the other.
- **The copy protection showed a Mech picture and asked a question about it**
  (`MW_MAIN` 081B:16A4..1819), before a new campaign starts. The rebuild leaves
  it out. It also drew random numbers, so leaving it out changes which random
  numbers the rest of the game sees. The rebuild's test recordings were made
  again after it was taken out.
- **One copy has a Mech that can't be loaded.** In the 1989 copy we have, the
  Phoenix Hawk's model file (`PHAPCK.TBL`) has the right length, but from byte
  0x1800 to 0x5FFF it holds the Marauder's model file instead, plus one flipped
  bit at 0x166D. Any battle with a Phoenix Hawk stops. Whether other 1989
  copies have the same damage isn't known. The rebuild's setup checks for it
  and names the file.

## Mechs and the market

- **The Wasp and the Wolverine were cut, but their stats are still there.**
  The campaign has eleven kinds of unit: ten Mechs and the DropShip. Its tables
  give the Wasp 20 tons, 95 km/h, 180 m jumps and a price of 2,000, and the
  Wolverine 55 tons, 86 km/h, 150 m jumps and 4,966, with heat sinks, weapons
  and ammunition like the others. Both even have their names in the shop's
  list. But when the market makes a Mech (`MW_MAIN` 081B:3BF7), a Wasp turns
  into a Locust and a Wolverine into a Phoenix Hawk, so you never see either.
  We haven't found models for them in the game's files.
- **That swap makes Locusts and Phoenix Hawks common.** The market draws a light
  Mech from three kinds (Locust, Wasp, Jenner) and a medium from three (Phoenix
  Hawk, Shadow Hawk, Wolverine). With the swap, two of the three light draws
  come out as a Locust, and two of the three medium ones as a Phoenix Hawk.
- **Every Mech for sale is used.** The market rolls damage onto each one,
  in three passes from worst to light (081B:3BF7). The price comes down by what
  repairing it would cost. Your company's first Jenner is damaged the same way,
  on purpose.
- **Prices depend on the world, and you can make money moving Mechs.** Each
  world has a market tier. A Mech costs its base price times 1.0, 1.2, 1.3 or
  1.5 by the tier, and the dealer buys it back at 0.1 less: 0.9, 1.1, 1.2 or 1.4
  (`MW_MAIN` 081B:4645 and the sale price beside it). Buy on a cheap world, sell
  on a dear one, and you keep up to 40%. On the dearer worlds the dealer only
  buys so many Mechs between restocks (the two dearest tiers). The rebuild keeps all of this as it was.
- **The DropShip's price uses the last repair quote you saw.** The price of
  any unit is worked out as its base price, minus the repair quote the game
  last calculated. For a Mech that's its own damage. The DropShip has no
  damage, but the game still subtracts whatever quote is left over, so the
  DropShip costs less if you've just looked at a badly damaged Mech.
- **Machine-gun ammunition is sold in batches of ten** in the reserve-ammo
  shop, charged by the round; every other kind is sold one at a time.

## The campaign

- **Your company's starting world is drawn at random** from a list of eight
  when a new campaign begins (`MW_MAIN` 081B:166D on).
- **House Marik's contract office always shows the same picture.** Each House
  has a list of its worlds, split into groups, and the group of the world
  you're on picks one of five regional pictures for the office. Marik's list
  has no splits at all: one group of 19, with one world in it twice. So every
  Marik world shows the first picture, and the world's second entry is never
  reached. It's the game's data as shipped.
- **Travel uses a ready-made table of square roots.** To turn the distance
  between two worlds into a travel time, the campaign looks up a square root in
  a table of 1,000 entries (`MW_MAIN` 1090:4CE6), entry *i* being the square
  root of 10 × *i*, rounded, and capped at 99. No formula we tried gives every
  entry. The best one matches 998 of the 1,000; entries 991 and 992 hold 98
  where it gives 99. The rebuild computes the table and keeps those two as
  measured.
- **Some data is never used.** There is a table of repair times by damage tier
  (`MW_MAIN` 1090:3A6F) that nothing reads, and a destination "time weight"
  for travel (1090:54DA) that is written but never read.

## Combat

- **Combat ran as fast as the PC could draw it.** The combat program has no
  clock of its own: it does one step of the battle, draws the cockpit, and goes
  straight on to the next step. A step's length came from how much the EGA card
  had to draw, about 14,000 bytes of screen a step. The port we started from
  modelled that, and its steps averaged about 127 milliseconds, under eight a
  second. On a faster PC the whole battle ran faster. The rebuild gives each step a fixed length,
  60 ms by default, and lets you speed it up to 4×.
- **Some waits did use real time.** Dialogs, the flash when you lose, and the
  orders screen's 76 ms minimum per frame are timed by the PC's timer chip, and
  keep their real lengths.
- **The radar reads the copyright notice when your lance has empty slots.**
  With fewer than four Mechs in your lance, the heading radar (`T`) still goes
  through all four slots, and for each empty one reads a "position" from
  address 0 plus 8 and 12 in the data segment. There, the original has the
  start of the Microsoft C runtime's copyright notice. So it reads two pairs
  of letters as map coordinates, and plots a dot only if they happen to fall
  inside the radar. The port we started from stopped with an error here; it now
  reads those same bytes.
- **Combat has music drivers, but no music.** The combat program has full
  drivers for the AdLib and for the Roland MT-32, but the game ships nothing for
  them to play in battle. Its sound file for combat holds only PC-speaker
  effects.
- **The joystick was timed by counting.** At every battle start, combat
  calibrates the joystick by counting how many game-port reads fit in a stretch
  of the timer chip. On a PC with no game port, every read fails and each axis
  waits out all 255 steps. In the port we started from, this took 0.83 seconds
  at every battle start. The rebuild reads joysticks through SDL instead.
- **The parts of your own Mech are hidden.** Your Mech exists in the 3D world
  like any other, but its torso, head, arms and legs are hidden, so you see
  from the cockpit without your own body in the way (`BTECH` 0810:25D4). A
  destroyed arm or leg is hidden the same way on other Mechs.

## Music and sound

- **The three campaign songs share one buffer.** The bar song is loaded at the
  start of a buffer, the death song 9,000 bytes in, and the launch song 11,000
  bytes in (`MW_MAIN` 081B:13A0..1422), and each load is allowed to read up to
  64 KB. If the bar song were ever longer than 9,000 bytes, loading it would run
  into the death song's place. The shipped files are short enough.
- **The MT-32 version uses the Roland's built-in sounds.** The songs never
  choose an instrument themselves. The MT-32 version picks the module's preset
  timbres with SysEx messages, and the AdLib version sets its own 11-byte FM
  instruments. The rebuild plays the songs from a General MIDI soundfont,
  with each part given the nearest instrument to the MT-32's.
- **Combat's AdLib instruments have a byte nobody reads.** The combat program's
  eight default AdLib instruments (`BTECH` 304E:2BA0) are 14 bytes each, but
  only the first 13 are used. The 14th is always zero.

## What comparing with the original caught

The rebuild runs scripted key presses through both our program and the
original under DOSBox-X, and compares the game's memory and screens at each
step. Bugs this caught in the port we started from were reported to its author:

- the radar's empty slots, above (an error stop where the original read on);
- the order screen stopping with an error when you have no lancemates;
- the retreat dialog's first line running on into the second (a string copy
  one byte short of its end);
- the travel map's destination name drawn in white instead of the yellow of a
  selected item (one buffer of the original's, copied into two in the port);
- combat failing on Linux when the game's files were in upper case.
