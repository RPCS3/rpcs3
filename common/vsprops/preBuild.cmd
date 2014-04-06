::@echo off
:: This file GENERATES the automatic GIT revision/version tag.
:: It uses the git.exe program to create an "svnrev.h" file for whichever
:: project is being compiled, during the project's pre-build step.
::
:: The git.exe program is part of the msysgit installation.
::
:: MsysGit can be downloaded from http://msysgit.github.io/
:: 
:: Usage: preBuild.cmd ProjectSrcDir VspropsDir
::
::    ProjectSrcDir - $(ProjectDir)\.. - Top-level Directory of project source code.

SETLOCAL ENABLEEXTENSIONS

set mydir=%~dp0

IF "%PROGRAMFILES(x86)%" == "" do (
  set PROGRAMFILES(x86)=%PROGRAMFILES%
)

set PATH=%PATH%;"%PROGRAMFILES(x86)%\Git\bin"

rem Test if git is available for this repo
git show -s
if %ERRORLEVEL% NEQ 0 (
  echo Automatic version detection unavailable.
  echo If you want to have the version string print correctly,
  echo make sure your Git.exe is in the default installation directory,
  echo or in your PATH.
  echo You can safely ignore this message - a dummy string will be printed.

  echo #define SVN_REV_UNKNOWN > "%CD%\svnrev.h"
  echo #define SVN_REV 0ll >> "%CD%\svnrev.h"
  echo #define SVN_MODS 0 >> "%CD%\svnrev.h"
  echo set SVN_REV=0 > "%CD%\postBuild.inc.cmd"
) else (

  FOR /F "delims=+" %%i IN ('"git show -s --format=%%%ci HEAD"') do (
    set REV3=%%i
  )
  set REV2=%REV3: =%
  set REV1=%REV2:-=%
  set REV=%REV1::=%

  echo #define SVN_REV %REV%ll > "%CD%\svnrev.h"
  echo #define SVN_MODS 0 /* Not implemented at the moment. */ >> "%CD%\svnrev.h"
  echo set SVN_REV=%REV% > "%CD%\postBuild.inc.cmd"
)

copy /Y "%mydir%\postBuild.unknown" "%CD%\postBuild.cmd"

ENDLOCAL
:: Always return an errorlevel of 0 -- this allows compilation to continue if SubWCRev failed.
exit /B 0
