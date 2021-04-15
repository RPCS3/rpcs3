# rpcs3 project build options
#========================================================================


# linker args
#========================================================================

option(
    USE_SYSTEM_ZLIB
    "Prefer system ZLIB instead of the builtin one"
    ON
)

option(
    USE_PRECOMPILED_HEADERS
    "Use precompiled headers"
    OFF
)

# build options
#========================================================================


# "USE_NATIVE_INSTRUCTIONS makes rpcs3 compile with -march=native,
#  which is useful for local builds, but not good for packages.
option(
    USE_NATIVE_INSTRUCTIONS
    "Enable march=native compatibility"
    ON
)

option(
    WITH_LLVM
    "Enable usage of LLVM library"
    ON
)

option(
    BUILD_LLVM_SUBMODULE
    "Build LLVM from git submodule"
    ON
)

option(
    USE_ALSA
    "Use ALSA audio backend"
    ON
)

option(
    USE_PULSE
    "Use PulseAudio audio backend"
    ON
)

option(
    USE_FAUDIO
    "Use FAudio audio backend"
    ON
)

option(
    USE_LIBEVDEV
    "Enable libevdev-based joystick support"
    ON
)

option(
    USE_DISCORD_RPC
    "Enable Discord rich presence integration"
    ON
)

option(
    USE_VULKAN
    "Enable Vulkan render backend"
    ON
)
