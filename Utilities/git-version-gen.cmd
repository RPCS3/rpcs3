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
rem // https://github.com/RPCS3/rpcs3 and https://rpcs3.net/.

setlocal ENABLEDELAYEDEXPANSION
setlocal ENABLEEXTENSIONS

set GIT_VERSION_FILE=%~p0..\rpcs3\git-version.h
if not defined GIT (
	set GIT="git"
)
call %GIT% describe > NUL 2> NUL
if errorlevel 1 (
	echo Git not on path, trying default Msysgit paths
	set GIT="%ProgramFiles(x86)%\Git\bin\git.exe"
	call !GIT! describe > NUL 2> NUL
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
	echo #define RPCS3_GIT_VERSION ^"local_build^" >> "%GIT_VERSION_FILE%"
	echo #define RPCS3_GIT_BRANCH ^"local_build^" >> "%GIT_VERSION_FILE%"
	echo #define RPCS3_GIT_FULL_BRANCH ^"local_build^" >> "%GIT_VERSION_FILE%"
	echo. >> "%GIT_VERSION_FILE%"
	echo // If you don't want this file to update/recompile, change to 1. >> "%GIT_VERSION_FILE%"
	echo #define RPCS3_GIT_VERSION_NO_UPDATE 0 >> "%GIT_VERSION_FILE%"
	goto done
)

rem // Get commit count from (unshallowed) HEAD
for /F %%I IN ('call %GIT% rev-list HEAD --count') do set COMMIT_COUNT=%%I

if defined SYSTEM_PULLREQUEST_SOURCEBRANCH (
	rem // These environment variables are defined by Azure pipelines
	rem // BUILD_REPOSITORY_NAME will look like "RPCS3/rpcs3"
	rem // SYSTEM_PULLREQUEST_SOURCEBRANCH will look like "master"
	rem // BUILD_SOURCEBRANCHNAME will look like "master"
	rem // See https://docs.microsoft.com/en-us/azure/devops/pipelines/build/variables
	set GIT_FULL_BRANCH=%BUILD_REPOSITORY_NAME%/%BUILD_SOURCEBRANCHNAME%

	if "%SYSTEM_PULLREQUEST_SOURCEBRANCH%"=="master" (
		rem // If pull request comes from a master branch, GIT_BRANCH = username/branch in order to distinguish from upstream/master
		for /f "tokens=1* delims=/" %%a in ("%BUILD_REPOSITORY_NAME%") do set user=%%a
		set "GIT_BRANCH=!user!/%SYSTEM_PULLREQUEST_SOURCEBRANCH%"
	) else (
		rem // Otherwise, GIT_BRANCH=branch
		set GIT_BRANCH=%SYSTEM_PULLREQUEST_SOURCEBRANCH%
	)

	rem // Make GIT_VERSION the last commit (shortened); Don't include commit count on non-master builds
	for /F %%I IN ('call %GIT% rev-parse --short^=8 HEAD') do set GIT_VERSION=%%I
) else (
	rem // Get last commit (shortened) and concat after commit count in GIT_VERSION
	for /F %%I IN ('call %GIT% rev-parse --short^=8 HEAD') do set GIT_VERSION=%COMMIT_COUNT%-%%I

	for /F %%I IN ('call %GIT% rev-parse --abbrev-ref HEAD') do set GIT_BRANCH=%%I

	set GIT_FULL_BRANCH=local_build
)

rem // Echo obtained GIT_VERSION for debug purposes if needed
echo %GIT_VERSION%
echo %GIT_FULL_BRANCH%

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
echo #define RPCS3_GIT_BRANCH ^"%GIT_BRANCH%^" >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_FULL_BRANCH ^"%GIT_FULL_BRANCH%^" >> "%GIT_VERSION_FILE%"
echo. >> "%GIT_VERSION_FILE%"
echo // If you don't want this file to update/recompile, change to 1. >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_VERSION_NO_UPDATE 0 >> "%GIT_VERSION_FILE%"

:done
