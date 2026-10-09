#define AppName "Futari Music"
#define AppStage GetEnv("FUTARI_STAGE_DIR")
#define OutputDir GetEnv("FUTARI_OUTPUT_DIR")
#define OutputName GetEnv("FUTARI_OUTPUT_NAME")
#define AppVersion GetEnv("FUTARI_CLIENT_VERSION")

[Setup]
AppId={{4F9D1792-7A37-4D66-A7A6-48E3112A92E7}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=ProgramCX
DefaultDirName={localappdata}\Programs\Futari Music
DefaultGroupName=Futari Music
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename={#OutputName}
SetupIconFile={#SourcePath}\..\..\icons\futari_icon.ico
UninstallDisplayIcon={app}\appFutariMusic.exe
CloseApplications=yes
RestartApplications=no
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "{#SourcePath}\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "{#AppStage}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Futari Music"; Filename: "{app}\appFutariMusic.exe"
Name: "{autodesktop}\Futari Music"; Filename: "{app}\appFutariMusic.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\appFutariMusic.exe"; Description: "{cm:LaunchProgram,Futari Music}"; Flags: postinstall nowait skipifsilent
