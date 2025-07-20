# .ci/build-windows-clang-cl.ps1
# Enable strict error handling
$ErrorActionPreference = "Stop"

trap {
    Write-Host "ERROR: $($_.Exception.Message)"
    exit 1
}

Write-Host "Starting RPCS3 build (PowerShell script)"

# Automatically find clang_rt.builtins-x86_64.lib
Write-Host "Searching for clang_rt.builtins-x86_64.lib ..."
$clangBuiltinsLibPath = Get-ChildItem -Path "C:/Program Files/LLVM/lib/clang" -Recurse -Filter "clang_rt.builtins-x86_64.lib" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "windows\\clang_rt\.builtins-x86_64\.lib$" } |
    Select-Object -First 1

if (-not $clangBuiltinsLibPath) {
    Write-Error "Could not find clang_rt.builtins-x86_64.lib in LLVM installation."
    exit 1
}

function Get-ShortPath([string]$path) {
    $fso = New-Object -ComObject Scripting.FileSystemObject
    return $fso.GetFolder($path).ShortPath
}

$clangBuiltinsDir = Split-Path -Parent $clangBuiltinsLibPath.FullName
$clangBuiltinsDirShort = Get-ShortPath $clangBuiltinsDir
$clangBuiltinsLib = Split-Path -Leaf $clangBuiltinsLibPath.FullName
$clangPath = "C:\Program Files\LLVM\bin"
#$clangPath = Get-ChildItem -Path "D:\a\rpcs3\rpcs3\llvm-*\bin"
#$llvmPath = Get-ChildItem -Path "D:\a\rpcs3\rpcs3\llvm-*\lib\cmake\llvm"

Write-Host "Found Clang builtins library: $clangBuiltinsLib in $clangBuiltinsDir or short $clangBuiltinsDirShort"
Write-Host "Found Clang Path: $clangPath"

# Get Windows Kits root from registry
$kitsRoot = Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" -Name "KitsRoot10"
$kitsRootPath = $kitsRoot.KitsRoot10

# Search for mt.exe in x64 SDK bin directories
Write-Host "Searching for mt.exe ..."
$mtPath = Get-ChildItem -Path "$clangPath" -Recurse -Filter "llvm-mt.exe" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\llvm-mt\.exe$" } |
    Sort-Object FullName -Descending |
    Select-Object -First 1

if (-not $mtPath) {
    Write-Error "Could not find mt.exe in Windows Kits directories."
    exit 1
}

$mtExePath = "$mtPath"

Write-Host "Found mt.exe at: $mtExePath"

$VcpkgRoot="$(Get-Location)/vcpkg"
$VcpkgTriplet=$env:VCPKG_TRIPLET
$VcpkgInstall="$VcpkgRoot/installed/$VcpkgTriplet"
$VcpkgInclude="$VcpkgInstall/include"
$VcpkgLib="$VcpkgInstall/lib"
$VcpkgBin="$VcpkgInstall/bin"

# Configure git safe directory
Write-Host "Configuring git safe directory"
& git config --global --add safe.directory '*'

# Initialize submodules except certain ones
Write-Host "Initializing submodules"
$excludedSubs = @('llvm','opencv','ffmpeg','FAudio','zlib','libpng','feralinteractive')

# Get submodule paths excluding those in $excludedSubs
$submodules = Select-String -Path .gitmodules -Pattern 'path = (.+)' | ForEach-Object {
    $_.Matches[0].Groups[1].Value
} | Where-Object {
    $path = $_
    -not ($excludedSubs | Where-Object { $path -like "*$_*" })
}

Write-Host "Updating submodules: $($submodules -join ', ')"
& git submodule update --init --quiet $submodules
& git -C 3rdparty/OpenAL/openal-soft reset --hard master

# Create and enter build directory
Write-Host "Creating build directory"
if (!(Test-Path build)) {
    New-Item -ItemType Directory -Path build | Out-Null
}
Set-Location build
Write-Host "Changed directory to: $(Get-Location)"

# Run CMake with Ninja generator and required flags
Write-Host "Running CMake configuration"
& cmake .. `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER="$clangPath/clang-cl.exe" `
    -DCMAKE_CXX_COMPILER="$clangPath/clang-cl.exe" `
    -DCMAKE_LINKER="$clangPath/lld-link.exe" `
    -DCMAKE_INSTALL_PREFIX=/usr `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
    -DCMAKE_EXE_LINKER_FLAGS="/LIBPATH:$clangBuiltinsDirShort /defaultlib:$clangBuiltinsLib" `
    -DCMAKE_MT="$clangPath/llvm-mt.exe" `
    -DUSE_NATIVE_INSTRUCTIONS=OFF `
    -DUSE_PRECOMPILED_HEADERS=OFF `
    -DVCPKG_TARGET_TRIPLET="$VcpkgTriplet" `
    -DFFMPEG_INCLUDE_DIR="$VcpkgInclude" `
    -DFFMPEG_LIBAVCODEC="$VcpkgLib/avcodec.lib" `
    -DFFMPEG_LIBAVFORMAT="$VcpkgLib/avformat.lib" `
    -DFFMPEG_LIBAVUTIL="$VcpkgLib/avutil.lib" `
    -DFFMPEG_LIBSWSCALE="$VcpkgLib/swscale.lib" `
    -DFFMPEG_LIBSWRESAMPLE="$VcpkgLib/swresample.lib" `
    -DUSE_SYSTEM_CURL=OFF `
    -DUSE_FAUDIO=OFF `
    -DUSE_SDL=ON `
    -DUSE_SYSTEM_SDL=OFF `
    -DUSE_SYSTEM_FFMPEG=ON `
    -DUSE_SYSTEM_OPENCV=ON `
    -DUSE_SYSTEM_OPENAL=OFF `
    -DUSE_SYSTEM_LIBPNG=ON `
    -DUSE_DISCORD_RPC=ON `
    -DUSE_SYSTEM_ZSTD=ON `
    -DWITH_LLVM=ON `
    -DSTATIC_LINK_LLVM=ON `
    -DBUILD_RPCS3_TESTS=OFF

Write-Host "CMake configuration complete"

# Build with ninja
Write-Host "Starting build with Ninja..."
& ninja
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed with exit code $LASTEXITCODE"
	exit 1
}

Write-Host "Build succeeded"

# Go back to root directory
Set-Location ..
Write-Host "Returned to root directory: $(Get-Location)"

