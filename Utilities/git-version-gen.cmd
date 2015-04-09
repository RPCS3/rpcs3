@echo off

rem // This program is free software: you can redistribute it and/or modify
rem // it under the terms of the GNU General Public License as published by
rem // the Free Software Foundation, version 2.0 or later versions.

rem // This program is distributed in the hope that it will be useful,
rem // but WITHOUT ANY WARRANTY; without even the implied warranty of
rem // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem // GNU General Public License 2.0 for more details.

rem // A copy of the GPL 2.0 should have been included with the program.
rem // If not, see http://www.gnu.org/licenses/

rem // Official git repository and contact information can be found at
rem // https://github.com/RPCS3/rpcs3 and http://rpcs3.net/.

setlocal ENABLEDELAYEDEXPANSION

set GIT_VERSION_FILE=%~p0..\rpcs3\git-version.h
if not defined GIT (
	set GIT="git"
)
call %GIT% describe > NUL 2> NUL
if errorlevel 1 (
	echo Git not on path, trying default Msysgit paths
	set GIT="%ProgramFiles(x86)%\Git\bin\git.exe"
	call %GIT% describe > NUL 2> NUL
	if errorlevel 1 (
		set GIT="%ProgramFiles%\Git\bin\git.exe"
	)
)

if exist "%GIT_VERSION_FILE%" (
	rem // Skip updating the file if RPCS3_GIT_VERSION_NO_UPDATE is 1.
	findstr /B /C:"#define RPCS3_GIT_VERSION_NO_UPDATE 1" "%GIT_VERSION_FILE%" > NUL
	if not errorlevel 1 (
		goto done
	)
)

call %GIT% describe --always > NUL 2> NUL
if errorlevel 1 (
	echo Unable to update git-version.h, git not found.
	echo If you don't want to add it to your path, set the GIT environment variable.

	echo // This is a generated file. > "%GIT_VERSION_FILE%"
	echo. >> "%GIT_VERSION_FILE%"
	echo #define RPCS3_GIT_VERSION "unknown" >> "%GIT_VERSION_FILE%"
	echo. >> "%GIT_VERSION_FILE%"
	echo // If you don't want this file to update/recompile, change to 1. >> "%GIT_VERSION_FILE%"
	echo #define RPCS3_GIT_VERSION_NO_UPDATE 0 >> "%GIT_VERSION_FILE%"
	goto done
)

for /F %%I IN ('call %GIT% describe --always') do set GIT_VERSION=%%I

rem // Don't modify the file if it already has the current version.
if exist "%GIT_VERSION_FILE%" (
	findstr /C:"%GIT_VERSION%" "%GIT_VERSION_FILE%" > NUL
	if not errorlevel 1 (
		goto done
	)
)

echo // This is a generated file. > "%GIT_VERSION_FILE%"
echo. >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_VERSION "%GIT_VERSION%" >> "%GIT_VERSION_FILE%"
echo. >> "%GIT_VERSION_FILE%"
echo // If you don't want this file to update/recompile, change to 1. >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_VERSION_NO_UPDATE 0 >> "%GIT_VERSION_FILE%"

:done
