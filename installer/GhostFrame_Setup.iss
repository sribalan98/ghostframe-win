; Script generated for Inno Setup 6
; GhostFrame - Stream Capture Cloaker Installer

#define MyAppName "GhostFrame"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "GhostFrame Project"
#define MyAppURL "https://github.com"
#define MyAppExeName "ghostframe-gui.exe"

[Setup]
AppId={{9B7E3E72-4F1A-4C2E-8E14-1A5314D8C47F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\dist
OutputBaseFilename=GhostFrame_Setup_v1.0
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
SetupIconFile=..\resources\ghostframe.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\build\bin\Release\ghostframe-gui.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\Release\ghostframe-helper64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\Release\ghostframe-payload64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\Release\ghostframe-test.exe"; DestDir: "{app}"; Flags: ignoreversion; Flags: skipifsourcedoesntexist
Source: "..\resources\ghostframe.ico"; DestDir: "{app}\resources"; Flags: ignoreversion; Flags: skipifsourcedoesntexist
Source: "..\src\icon\ghostframe.png"; DestDir: "{app}\resources"; Flags: ignoreversion; Flags: skipifsourcedoesntexist
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion; Flags: skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
