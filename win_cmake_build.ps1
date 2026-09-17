#Requires -Version 5.1
param(
    [int]$Cores = 0
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Clear-Host

$StartTime = Get-Date

# ============================================================
# Visual Studio Detection
# ============================================================
$VsVarsPath = $null

$UninstallKeys = @(
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall',
    'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall'
)

:vsSearch foreach ($BaseKey in $UninstallKeys) {
    foreach ($SubKey in (Get-ChildItem $BaseKey -ErrorAction SilentlyContinue)) {
        $DisplayName = $SubKey.GetValue('DisplayName', '')
        if ($DisplayName -like '*Visual Studio*') {
            $InstallLocation = $SubKey.GetValue('InstallLocation', '')
            if ($InstallLocation) {
                $Candidate = Join-Path $InstallLocation 'VC\Auxiliary\Build\vcvars64.bat'
                if (Test-Path $Candidate) {
                    $VsVarsPath = $Candidate
                    break vsSearch
                }
            }
        }
    }
}

if (-not $VsVarsPath) {
    Write-Error 'ERROR: Visual Studio not found.'
    Read-Host 'Press Enter to exit'
    exit 1
}

Write-Host "Using VS environment: $VsVarsPath"

# Import VS environment variables by running vcvars64.bat and capturing the env
$TempFile = [System.IO.Path]::GetTempFileName() + '.bat'
@"
@echo off
call "$VsVarsPath"
set
"@ | Set-Content $TempFile -Encoding ASCII

cmd /c $TempFile | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}
Remove-Item $TempFile -Force

# ============================================================
# Qt Detection
# ============================================================
$QtRoot = $null

if ($env:Qt6_ROOT -and (Test-Path (Join-Path $env:Qt6_ROOT 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    $QtRoot = $env:Qt6_ROOT
    Write-Host "Found Qt6 from environment variable: $env:Qt6_ROOT"
}

if (-not $QtRoot) {
    foreach ($Drive in 'C','D','E','F','G','H') {
        $QtBase = "${Drive}:\Qt"
        if (-not (Test-Path $QtBase)) { continue }
        foreach ($VersionDir in (Get-ChildItem $QtBase -Directory -Filter '6.*' -ErrorAction SilentlyContinue)) {
            $Candidate = Join-Path $VersionDir.FullName 'msvc2022_64'
            if (Test-Path (Join-Path $Candidate 'lib\cmake\Qt6\Qt6Config.cmake')) {
                $QtRoot = $Candidate
                Write-Host "Found Qt6 at: $Candidate"
                break
            }
        }
        if ($QtRoot) { break }
    }
}

if (-not $QtRoot) {
    Write-Error @'
ERROR: Qt6 not found. Please set the Qt6_ROOT environment variable.
Example: [System.Environment]::SetEnvironmentVariable('Qt6_ROOT','C:\Qt\6.8.3\msvc2022_64','User')
'@
    Read-Host 'Press Enter to exit'
    exit 1
}

# ============================================================
# Build Configuration
# ============================================================
if ($Cores -le 0) {
    $Cores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
}

$SourceDir = $PSScriptRoot
$BuildDir  = Join-Path $SourceDir 'build'

# ============================================================
# Selective Clean (SDL3 export conflict fix)
# Only wipe SDL3 folder to avoid the export() CMake 4.x conflict
# while preserving all other compiled objects (LLVM, ffmpeg, etc.)
# ============================================================
$SdlDir = Join-Path $BuildDir '3rdparty\libsdl-org'
if (Test-Path $SdlDir) {
    Write-Host ''
    Write-Host 'Cleaning SDL3 build cache to prevent export conflicts...'
    Remove-Item $SdlDir -Recurse -Force
    Write-Host 'SDL3 clean done.'
}

if (-not (Test-Path $BuildDir)) {
    New-Item $BuildDir -ItemType Directory | Out-Null
}

# ============================================================
# CMake Configure
# Skipped if CMakeCache.txt already exists from a previous run.
# Delete build\CMakeCache.txt or the entire build folder to force
# a full reconfigure (e.g. after changing cmake flags).
# ============================================================
$CacheFile = Join-Path $BuildDir 'CMakeCache.txt'
if (-not (Test-Path $CacheFile)) {
    Write-Host ''
    Write-Host "Configuring CMake with $Cores cores..."
    Write-Host "Qt6 Root: $QtRoot"
    Write-Host ''

    & cmake $SourceDir -G 'Ninja Multi-Config' `
        -DBUILD_LLVM=ON `
        -DUSE_SYSTEM_OPENCV=OFF `
        -DUSE_SYSTEM_CURL=OFF `
        -DUSE_SYSTEM_ZLIB=OFF `
        -DUSE_SYSTEM_LIBPNG=OFF `
        -DUSE_SYSTEM_SDL=OFF `
        -DUSE_VULKAN=ON `
        "-DQt6_ROOT=$QtRoot" `
        "-DCMAKE_PREFIX_PATH=$QtRoot" `
        -B $BuildDir

    if ($LASTEXITCODE -ne 0) {
        Write-Host ''
        Write-Error 'ERROR: CMake configuration failed!'
        Print-Elapsed $StartTime
        Read-Host 'Press Enter to exit'
        exit 1
    }
} else {
    Write-Host ''
    Write-Host 'Skipping CMake configure (cache exists).'
    Write-Host "To force a full reconfigure, delete: $CacheFile"
}

# ============================================================
# Build
# Ninja will only recompile files that have actually changed.
# ============================================================
Write-Host ''
Write-Host "Building with $Cores cores..."

& cmake --build $BuildDir --config Release --parallel $Cores

if ($LASTEXITCODE -ne 0) {
    Write-Host ''
    Write-Error 'ERROR: Build failed!'
    Print-Elapsed $StartTime
    Read-Host 'Press Enter to exit'
    exit 1
}

Write-Host ''
Write-Host "Build complete! Output is in `"$BuildDir\bin\`""
Print-Elapsed $StartTime
Read-Host 'Press Enter to exit'
exit 0

# ============================================================
# Helper: print elapsed time
# ============================================================
function Print-Elapsed {
    param([datetime]$Start)
    $Elapsed = (Get-Date) - $Start
    Write-Host ''
    Write-Host ('Total build time: {0}h {1}m {2}s' -f
        [int]$Elapsed.TotalHours,
        $Elapsed.Minutes,
        $Elapsed.Seconds)
}
