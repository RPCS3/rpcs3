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
:: Build Configuration
:: ============================================================
set CORES=%1
set "SOURCE_DIR=%~dp0"
set "SOURCE_DIR=%SOURCE_DIR:~0,-1%"
set "BUILD_DIR=%SOURCE_DIR%\build"

:: ============================================================
:: Clean old build folder to avoid stale CMake cache issues
:: ============================================================
if exist "%BUILD_DIR%" (
    echo.
    echo Cleaning old build folder...
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo ERROR: Could not delete old build folder. Close any programs using it and try again.
        pause
        exit /b 1
    )
    echo Clean done.
)

:: Create fresh build dir
mkdir "%BUILD_DIR%"

:: ============================================================
:: CMake Configure
:: ============================================================
echo.
echo Configuring CMake...
cmake "%SOURCE_DIR%" -G "Visual Studio 18 2026" -A x64 ^
    -DBUILD_LLVM=ON ^
    -DUSE_SYSTEM_OPENCV=OFF ^
    -DUSE_SYSTEM_CURL=OFF ^
    -DUSE_SYSTEM_ZLIB=OFF ^
    -DUSE_SYSTEM_LIBPNG=OFF ^
    -DUSE_SYSTEM_SDL=OFF ^
    -DUSE_VULKAN=ON ^
    -B "%BUILD_DIR%"

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

:: ============================================================
:: Build
:: ============================================================
echo.
if "%CORES%"=="" (
    echo Building with default core count...
    cmake --build "%BUILD_DIR%" --config Release
) else (
    echo Building with %CORES% cores...
    cmake --build "%BUILD_DIR%" --config Release --parallel %CORES%
)

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Build complete! Output is in "%BUILD_DIR%\bin\"
pause