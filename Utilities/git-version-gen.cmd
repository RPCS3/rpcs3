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
	echo GIT not defined, using git as command.
	set GIT='git'
	echo PATH is:
	echo "%PATH%"
) else (
	echo GIT defined as %GIT%
)

call %GIT% describe --always > NUL 2> NUL
if errorlevel 1 (
	echo Git not on PATH, trying fallback paths

	set git_array[0]="%ProgramFiles%\Git\bin\git.exe"
	set git_array[1]="%ProgramFiles(x86)%\Git\bin\git.exe"
	set git_array[2]="%ProgramW6432%\Git\bin\git.exe"
	set git_array[3]="C:\Program Files (x86)\Git\bin\git.exe"
	set git_array[4]="C:\Program Files\Git\bin\git.exe"
	set git_array[5]="%ProgramFiles%\Git\cmd\git.exe"
	set git_array[6]="%ProgramFiles(x86)%\Git\cmd\git.exe"
	set git_array[7]="%ProgramW6432%\Git\cmd\git.exe"
	set git_array[8]="C:\Program Files (x86)\Git\cmd\git.exe"
	set git_array[9]="C:\Program Files\Git\cmd\git.exe"

	for /l %%n in (0,1,9) do (
		set GIT=!git_array[%%n]!
		echo Trying fallback path !GIT!
		call !GIT! describe --always > NUL 2> NUL
		if errorlevel 1 (
			echo git not found on path !GIT!
		) else (
			echo git found on path !GIT!
			goto end_of_git_loop
		)
	)
	echo git not found
)
:end_of_git_loop

if exist "%GIT_VERSION_FILE%" (
	rem // Skip updating the file if RPCS3_GIT_VERSION_NO_UPDATE is 1.
	findstr /B /C:"#define RPCS3_GIT_VERSION_NO_UPDATE 1" "%GIT_VERSION_FILE%" > NUL
	if not errorlevel 1 (
		echo Skip updating the file: RPCS3_GIT_VERSION_NO_UPDATE is 1.
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
	echo defined SYSTEM_PULLREQUEST_SOURCEBRANCH
	echo SYSTEM_PULLREQUEST_SOURCEBRANCH: %SYSTEM_PULLREQUEST_SOURCEBRANCH%
	echo BUILD_REPOSITORY_NAME: %BUILD_REPOSITORY_NAME%
	echo BUILD_SOURCEBRANCHNAME: %BUILD_SOURCEBRANCHNAME%

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
	echo not defined SYSTEM_PULLREQUEST_SOURCEBRANCH
	rem // Get last commit (shortened) and concat after commit count in GIT_VERSION
	for /F %%I IN ('call %GIT% rev-parse --short^=8 HEAD') do set GIT_VERSION=%COMMIT_COUNT%-%%I

	for /F %%I IN ('call %GIT% rev-parse --abbrev-ref HEAD') do set GIT_BRANCH=%%I

	set GIT_FULL_BRANCH=local_build
)

rem // Echo obtained GIT_VERSION for debug purposes if needed
echo final variables:
if defined GIT_VERSION (
	echo GIT_VERSION: %GIT_VERSION%
) else (
	echo GIT_VERSION undefined
)
if defined GIT_BRANCH (
	echo GIT_BRANCH: %GIT_BRANCH%
) else (
	echo GIT_BRANCH undefined
)
if defined GIT_FULL_BRANCH (
	echo GIT_FULL_BRANCH: %GIT_FULL_BRANCH%
) else (
	echo GIT_FULL_BRANCH undefined
)

rem // Don't modify the file if it already has the current version.
if exist "%GIT_VERSION_FILE%" (
	findstr /C:"%GIT_VERSION%" "%GIT_VERSION_FILE%" > NUL
	if not errorlevel 1 (
		echo done, already has the current version...
		goto done
	)
)

echo // This is a generated file. > "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_VERSION "%GIT_VERSION%" >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_BRANCH ^"%GIT_BRANCH%^" >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_FULL_BRANCH ^"%GIT_FULL_BRANCH%^" >> "%GIT_VERSION_FILE%"
echo // If you don't want this file to update/recompile, change to 1. >> "%GIT_VERSION_FILE%"
echo #define RPCS3_GIT_VERSION_NO_UPDATE 0 >> "%GIT_VERSION_FILE%"

:done

echo File is:
more %GIT_VERSION_FILE%
echo File end
