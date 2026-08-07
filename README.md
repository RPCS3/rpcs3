PS3Native
=========

A PlayStation 3 emulator for Android, built directly on top of current upstream [RPCS3](https://github.com/RPCS3/rpcs3).

This fork tracks the real RPCS3 tree rather than a snapshot. The archived [RPCS3-Android](https://github.com/RPCS3/rpcs3-android) port is folded in at [`android/`](android), rebased onto current upstream, and carried forward with additional fixes and interface work contributed by the [WinNative](https://github.com/WinNative-Emu) developers.

## What this is

Upstream RPCS3 targets desktop platforms. The Android port was published separately and then went stale against a moving upstream. The goal here is a single tree where:

* the emulator core is unmodified upstream RPCS3, so improvements land by merging upstream rather than by re-porting;
* the Android front end lives beside it in `android/` and builds an APK from that same source;
* Android-specific defects — threading, shutdown, configuration persistence, on-screen controls — are fixed in place;
* the interface is a Compose Material 3 front end sharing the design language of WinNative.

Everything below `android/` is additive. The upstream history is preserved intact.

## State

Working:

* boots and runs commercial titles at full speed on current Snapdragon hardware
* Vulkan renderer, PPU/SPU recompilers via a cross-compiled LLVM
* per-game configuration, backed by RPCS3's own custom-config mechanism
* generated PS2-style on-screen controls with sticky presses and reserved touch zones
* in-game menu with pause/resume, settings and shutdown
* game library with a built-in compatibility browser

Not finished:

* netplay has never been verified end to end; RPCN has no sign-in interface on Android
* an intermittent crash in the render queue is still being tracked
* audio backend selection is limited

## Building

The Android build needs LLVM and FFmpeg cross-compiled for `arm64-v8a` first. Both scripts fetch their own sources and write into `android/prebuilt/`:

```bash
cd android
./build-ffmpeg-android.sh
./build-llvm-android.sh
./gradlew assembleStandardDebug
```

Requirements: JDK 17, Android NDK 29, CMake 3.31, and a checkout with submodules (`git submodule update --init --recursive`).

The dependency scripts are the slow part and only need running when they change. `android/prebuilt/` is not tracked.

To build in CI, run the **Android APK** workflow manually from the Actions tab. It is dispatch-only, caches the cross-compiled dependencies against the build-script hashes, and shares a ccache across both the dependency and application builds, so a run that only touches app code reuses everything else.

## Flavors

The same APK is published under several package names. Some Android vendors gate their high-performance CPU and GPU governors on an allowlist of package names, so a build installed under one of those identifiers is scheduled more aggressively. Pick whichever performs best on your device.

| Flavor | Package name | Gradle task |
| --- | --- | --- |
| `standard` | `com.ps3native.standard` | `assembleStandardDebug` |
| `antutu` | `com.antutu.ABenchMark` | `assembleAntutuDebug` |
| `ludashi` | `com.ludashi.benchmark` | `assembleLudashiDebug` |
| `pubg` | `com.tencent.ig` | `assemblePubgDebug` |

All four are the same emulator and carry the same name and icon. Because a package name is unique on a device, a flavor cannot be installed alongside the real application that owns that identifier, and only `standard` is suitable for distribution through an app store.

## Credits

* the [RPCS3](https://github.com/RPCS3/rpcs3) team, for the emulator
* the [RPCS3-Android](https://github.com/RPCS3/rpcs3-android) port this build started from
* the [WinNative](https://github.com/WinNative-Emu) developers, for the interface work and Android fixes

## License

Most files are licensed under GNU GPL-2.0-only; see [LICENSE](LICENSE). Some files are licensed differently — check the individual file headers. This project is not affiliated with or endorsed by Sony Interactive Entertainment.
