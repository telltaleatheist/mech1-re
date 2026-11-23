; The Windows installer for players: Inno Setup (jrsoftware.org, free), built
; by tools/build_package.py --installer from the package folder it has just
; staged:
;   iscc /DAppVersion=<label> /DDist=<package folder> /DStem=<file stem> /O<out dir> tools\package\installer.iss
; It installs to Program Files\MechWarrior 1 Rebuild with a Start-menu
; shortcut (a desktop one ticked by default) and an uninstaller. The game
; writes nothing there: the player's settings (MechWarrior.ini), the chosen
; game folder, saves, story/ and the log live in
; %APPDATA%\BattleTechReverse\MechWarrior, which the uninstaller leaves alone.
; No file of the original game is in the installer.

#ifndef AppVersion
  #define AppVersion "dev"
#endif
#ifndef Dist
  #error Give /DDist=<the package folder> (build_package.py does).
#endif
#ifndef Stem
  #define Stem "MW1-Rebuild-" + AppVersion
#endif

[Setup]
; This project's own AppId, the same every version: a new installer upgrades
; the old install in place.
AppId={{56BFB0C4-541E-42DD-909E-874C8EDA0D47}
AppName=MechWarrior 1 Rebuild
AppVersion={#AppVersion}
AppVerName=MechWarrior 1 Rebuild {#AppVersion}
AppPublisher=MechWarrior 1 Rebuild (a fan project)
AppComments=A rebuild of MechWarrior (1989)'s program; it needs your own copy of the game.
DefaultDirName={autopf}\MechWarrior 1 Rebuild
; The folder page every time, an upgrade's too, offering the folder already
; installed to.
DisableDirPage=no
DefaultGroupName=MechWarrior 1 Rebuild
DisableProgramGroupPage=yes
; Program Files needs administrator rights; a player without them can pick
; "only for me" (the user's own Programs folder) in the first dialog.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=mechwarrior.ico
UninstallDisplayIcon={app}\mechwarrior.exe
UninstallDisplayName=MechWarrior 1 Rebuild
OutputBaseFilename={#Stem}-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
#if FileExists(AddBackslash(Dist) + "LICENSE")
LicenseFile={#Dist}\LICENSE
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#Dist}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
; data\ is ours and replaced whole, so a file an earlier version shipped and
; this one dropped doesn't linger. Nothing else in {app} is touched.
Type: filesandordirs; Name: "{app}\data"

[Icons]
Name: "{autoprograms}\MechWarrior 1 Rebuild"; Filename: "{app}\mechwarrior.exe"; WorkingDir: "{app}"; Comment: "MechWarrior (1989), rebuilt"
Name: "{autoprograms}\MechWarrior 1 Rebuild - Read me"; Filename: "{app}\README.txt"
Name: "{autodesktop}\MechWarrior 1 Rebuild"; Filename: "{app}\mechwarrior.exe"; WorkingDir: "{app}"; Comment: "MechWarrior (1989), rebuilt"; Tasks: desktopicon

[Run]
Filename: "{app}\mechwarrior.exe"; Description: "{cm:LaunchProgram,MechWarrior 1 Rebuild}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Only what the installer put in {app}; the player's folder in %APPDATA% stays.
Type: filesandordirs; Name: "{app}\data"
Type: dirifempty; Name: "{app}"

[Messages]
UninstalledAll=%1 was removed from your computer.%n%nYour settings, saved games and the game folder you chose are kept in %APPDATA%\BattleTechReverse\MechWarrior. Delete that folder to remove them too. Your copy of MechWarrior itself was never changed.
