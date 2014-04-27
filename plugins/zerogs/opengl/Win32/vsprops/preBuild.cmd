@echo off

rem Usage: preBuild.cmd ProjectSrcDir VspropsDir
rem
rem    ProjectSrcDir - $(ProjectDir)\.. - Directory of project source code.
rem    VspropsDir - $(PrjectDir)\vsprops - Directory of this script and its counterparts.

SubWCRev.exe %~1 %~2\svnrev_template.h %~1\svnrev.h
if %ERRORLEVEL% NEQ 0 (
  echo Automatic revision update unavailable, using generic template instead.
  echo You can safely ignore this message - see svnrev.h for details.
  copy /Y %~2\svnrev_unknown.h %~1\svnrev.h
  copy /Y %~2\postBuild.unknown %~2\postBuild.cmd
) else (
  SubWCRev.exe %~1 %~2\postBuild.tmpl %~2\postBuild.cmd
)

rem Always return an errorlevel of 0 -- this allows compilation to continue if SubWCRev failed.

exit 0
