@echo off
setlocal enabledelayedexpansion
cls

:: ============================================================
:: Visual Studio Detection
:: ============================================================
set "VSVARS_PATH="

for /f "tokens=*" %%a in ('
    reg query "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall" /s /f "Visual Studio" ^|
    findstr "HKEY"
') do (
    for /f "tokens=2*" %%b in ('
        reg query "%%a" /v InstallLocation 2^>nul ^|
        findstr /i "InstallLocation"
    ') do (
        if not "%%c"=="" (
            if exist "%%c\VC\Auxiliary\Build\vcvars64.bat" (
                set "VSVARS_PATH=%%c\VC\Auxiliary\Build\vcvars64.bat"
                goto :found_vs
            )
        )
    )
)

:found_vs
if not defined VSVARS_PATH (
    echo ERROR: Visual Studio not found.
    pause
    exit /b 1
)

echo Using VS environment: %VSVARS_PATH%
call "%VSVARS_PATH%"

:: ============================================================
:: Qt Detection
:: ============================================================
set "QT_ROOT="

if defined Qt6_ROOT (
    if exist "%Qt6_ROOT%\lib\cmake\Qt6\Qt6Config.cmake" (
        set "QT_ROOT=%Qt6_ROOT%"
        echo Found Qt6 from environment variable: %Qt6_ROOT%
        goto :found_qt
    )
)

for %%D in (C D E F G H) do (
    for /d %%Q in ("%%D:\Qt\6.*") do (
        if exist "%%Q\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" (
            set "QT_ROOT=%%Q\msvc2022_64"
            echo Found Qt6 at: %%Q\msvc2022_64
            goto :found_qt
        )
    )
)

echo ERROR: Qt6 not found. Please set the Qt6_ROOT environment variable.
echo Example: setx Qt6_ROOT "C:\Qt\6.8.3\msvc2022_64"
pause
exit /b 1

:found_qt

:: ============================================================
:: Build Configuration
:: ============================================================
if "%~1"=="" (
    for /f "tokens=2 delims==" %%a in (
        'wmic cpu get NumberOfLogicalProcessors /value ^| findstr "="'
    ) do set CORES=%%a
) else (
    set CORES=%1
)

set "SOURCE_DIR=%~dp0"
set "SOURCE_DIR=%SOURCE_DIR:~0,-1%"
set "BUILD_DIR=%SOURCE_DIR%\build"

set START_TIME=%TIME%

:: ============================================================
:: Selective Clean (SDL3 export conflict fix)
:: Only wipe SDL3 folder to avoid the export() CMake 4.x conflict
:: while preserving all other compiled objects (LLVM, ffmpeg, etc.)
:: ============================================================
if exist "%BUILD_DIR%\3rdparty\libsdl-org" (
    echo.
    echo Cleaning SDL3 build cache to prevent export conflicts...
    rmdir /s /q "%BUILD_DIR%\3rdparty\libsdl-org"
    echo SDL3 clean done.
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ============================================================
:: CMake Configure
:: Skipped if CMakeCache.txt already exists from a previous run.
:: Delete build\CMakeCache.txt or the entire build folder to force
:: a full reconfigure (e.g. after changing cmake flags).
:: ============================================================
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo.
    echo Configuring CMake with %CORES% cores...
    echo Qt6 Root: %QT_ROOT%
    echo.

    cmake "%SOURCE_DIR%" -G "Ninja Multi-Config" ^
        -DBUILD_LLVM=ON ^
        -DUSE_SYSTEM_OPENCV=OFF ^
        -DUSE_SYSTEM_CURL=OFF ^
        -DUSE_SYSTEM_ZLIB=OFF ^
        -DUSE_SYSTEM_LIBPNG=OFF ^
        -DUSE_SYSTEM_SDL=OFF ^
        -DUSE_VULKAN=ON ^
        -DQt6_ROOT="%QT_ROOT%" ^
        -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
        -B "%BUILD_DIR%"

    if errorlevel 1 (
        echo.
        echo ERROR: CMake configuration failed!
        call :print_time
        pause
        exit /b 1
    )
) else (
    echo.
    echo Skipping CMake configure ^(cache exists^).
    echo To force a full reconfigure, delete: %BUILD_DIR%\CMakeCache.txt
)

:: ============================================================
:: Build
:: Ninja will only recompile files that have actually changed.
:: ============================================================
echo.
echo Building with %CORES% cores...
cmake --build "%BUILD_DIR%" --config Release --parallel %CORES% -- -j%CORES%

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    call :print_time
    pause
    exit /b 1
)

echo.
echo Build complete! Output is in "%BUILD_DIR%\bin\"
call :print_time
pause
exit /b 0

:print_time
set END_TIME=%TIME%
for /f "tokens=1-4 delims=:., " %%a in ("%START_TIME%") do set /a START_S=%%a*3600+%%b*60+%%c
for /f "tokens=1-4 delims=:., " %%a in ("%END_TIME%") do set /a END_S=%%a*3600+%%b*60+%%c
set /a ELAPSED=END_S-START_S
if %ELAPSED% lss 0 set /a ELAPSED+=86400
set /a HOURS=ELAPSED/3600
set /a MINS=(ELAPSED%%3600)/60
set /a SECS=ELAPSED%%60
echo.
echo Total build time: %HOURS%h %MINS%m %SECS%s
goto :eof
