MechWarrior 1 Rebuild

A rebuild of MechWarrior (1989)'s program. Your own copy of the game is required and is
not included: no game data, saves or MT-32 ROMs are in this package.

The first time it starts it asks for the folder that holds your copy of MechWarrior (the
one with MW_MAIN.EXE), reads the game's programs once and remembers the folder. Your
settings (MechWarrior.ini), saves and the log (mechwarrior.log) live in your own folder:
  Windows: %APPDATA%\BattleTechReverse\MechWarrior
  macOS:   ~/Library/Application Support/BattleTechReverse/MechWarrior
  Linux:   ~/.local/share/BattleTechReverse/MechWarrior
To choose another game folder, delete the game_folder line from that MechWarrior.ini.

Keys: arrows and Enter; in combat - and + change the speed (1x to 4x); F12 saves a
screenshot beside your saves; Ctrl+Escape quits.

From a command line: mechwarrior <game folder> [capture folder] starts on that folder.
