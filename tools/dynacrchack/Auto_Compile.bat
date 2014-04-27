@echo off
set source=DynaCrcHack.c
set tcc=tcc\tcc

pushd "%~dp0"

if exist %tcc%.exe goto pre

rem local tcc not found, try to invoke a global tcc
set tcc=tcc
%tcc% utils\ding.c -luser32 -o utils\ding.exe >nul 2>nul
if %errorlevel% == 0 (
		echo.
		echo Using globally installed tcc ...
		echo.
) else (
		echo.
		echo Missing ^<this-folder^>\tcc\tcc.exe
		echo.
		echo Please download TCC 0.9.25 for windows from http://bellard.org/tcc/
		echo and extract the package to ^<this-folder^>\tcc
		echo.
		pause
		goto end
)

:pre
if not exist utils\waitForChange.exe	%tcc% utils\waitForChange.c -o utils\waitForChange.exe
if not exist utils\ding.exe				%tcc% utils\ding.c -luser32 -o utils\ding.exe

:start
echo Compiling ...
echo.
%tcc% -shared -Wall %source%
if %errorlevel% == 0 (
	echo -^> OK
	utils\ding 2
) else (
	echo -^> ERROR
	utils\ding 1
	utils\ding 1
)

if exist *.def del /Q *.def

echo Waiting ...
utils\waitForChange %source%
echo.

goto start

:end
popd