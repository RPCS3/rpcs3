; Basic setup script for the Inno Setep installer builder.  For more
; information on the free installer builder, see www.jrsoftware.org.
;
; This script was contributed by Tim Peters.
; The current version is used with Inno Setup 2.0.19.

[Setup]
AppName=expat
AppId=expat
AppVersion=1.95.6
AppVerName=expat 1.95.6
AppCopyright=Copyright © 1998-2002 Thai Open Source Software Center, Clark Cooper, and the Expat maintainers
DefaultDirName={sd}\Expat-1.95.6
AppPublisher=The Expat Developers
AppPublisherURL=http://www.libexpat.org/
AppSupportURL=http://www.libexpat.org/
AppUpdatesURL=http://www.libexpat.org/
UninstallDisplayName=Expat XML Parser (version 1.95.6)
UninstallFilesDir={app}\Uninstall

Compression=bzip/9
SourceDir=..
OutputDir=win32
DisableStartupPrompt=yes
AllowNoIcons=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes

[Files]
CopyMode: alwaysoverwrite; Source: xmlwf\Release\*.exe;        DestDir: "{app}"
CopyMode: alwaysoverwrite; Source: win32\MANIFEST.txt;         DestDir: "{app}"
CopyMode: alwaysoverwrite; Source: Changes;                    DestDir: "{app}"; DestName: Changes.txt
CopyMode: alwaysoverwrite; Source: COPYING;                    DestDir: "{app}"; DestName: COPYING.txt
CopyMode: alwaysoverwrite; Source: README;                     DestDir: "{app}"; DestName: README.txt
CopyMode: alwaysoverwrite; Source: doc\*.html;                 DestDir: "{app}\Doc"
CopyMode: alwaysoverwrite; Source: doc\*.css;                  DestDir: "{app}\Doc"
CopyMode: alwaysoverwrite; Source: doc\*.png;                  DestDir: "{app}\Doc"
CopyMode: alwaysoverwrite; Source: lib\Release\*.dll;          DestDir: "{app}\Libs"
CopyMode: alwaysoverwrite; Source: lib\Release\*.lib;          DestDir: "{app}\Libs"
CopyMode: alwaysoverwrite; Source: lib\Release-w\*.dll;        DestDir: "{app}\Libs"
CopyMode: alwaysoverwrite; Source: lib\Release-w\*.lib;        DestDir: "{app}\Libs"
CopyMode: alwaysoverwrite; Source: lib\Release_static\*.lib;   DestDir: "{app}\StaticLibs"
CopyMode: alwaysoverwrite; Source: lib\Release-w_static\*.lib; DestDir: "{app}\StaticLibs"
CopyMode: alwaysoverwrite; Source: expat.dsw;                  DestDir: "{app}\Source"
CopyMode: alwaysoverwrite; Source: bcb5\*.*;                   DestDir: "{app}\Source\bcb5"
CopyMode: alwaysoverwrite; Source: lib\*.c;                    DestDir: "{app}\Source\lib"
CopyMode: alwaysoverwrite; Source: lib\*.h;                    DestDir: "{app}\Source\lib"
CopyMode: alwaysoverwrite; Source: lib\*.def;                  DestDir: "{app}\Source\lib"
CopyMode: alwaysoverwrite; Source: lib\*.dsp;                  DestDir: "{app}\Source\lib"
CopyMode: alwaysoverwrite; Source: examples\*.c;               DestDir: "{app}\Source\examples"
CopyMode: alwaysoverwrite; Source: examples\*.dsp;             DestDir: "{app}\Source\examples"
CopyMode: alwaysoverwrite; Source: tests\*.c;                  DestDir: "{app}\Source\tests"
CopyMode: alwaysoverwrite; Source: tests\*.h;                  DestDir: "{app}\Source\tests"
CopyMode: alwaysoverwrite; Source: tests\README.txt;           DestDir: "{app}\Source\tests"
CopyMode: alwaysoverwrite; Source: xmlwf\*.c*;                 DestDir: "{app}\Source\xmlwf"
CopyMode: alwaysoverwrite; Source: xmlwf\*.h;                  DestDir: "{app}\Source\xmlwf"
CopyMode: alwaysoverwrite; Source: xmlwf\*.dsp;                DestDir: "{app}\Source\xmlwf"

[Messages]
WelcomeLabel1=Welcome to the Expat XML Parser Setup Wizard
WelcomeLabel2=This will install [name/ver] on your computer.%n%nExpat is an XML parser with a C-language API, and is primarily made available to allow developers to build applications which use XML using a portable API and fast implementation.%n%nIt is strongly recommended that you close all other applications you have running before continuing. This will help prevent any conflicts during the installation process.
