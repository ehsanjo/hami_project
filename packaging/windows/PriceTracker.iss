; Inno Setup script for PriceTracker.
; Expects the deployed app (exe + Qt DLLs + plugins) in ..\..\dist\windows\app
; (build-windows.bat creates that folder with windeployqt).

#define MyAppName "PriceTracker"
#define MyAppVersion "1.0.0"
#define MyAppExe "PriceTracker.exe"

[Setup]
AppId={{B3E0F2C4-6A57-4D1E-9C0B-5E7A1D2F8A31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExe}
SetupIconFile=..\icon.ico
OutputDir=..\..\dist\windows
OutputBaseFilename=PriceTracker-{#MyAppVersion}-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked
Name: "startup"; Description: "Start PriceTracker when I sign in"; GroupDescription: "Options:"; Flags: unchecked

[Files]
Source: "..\..\dist\windows\app\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: desktopicon
Name: "{userstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
