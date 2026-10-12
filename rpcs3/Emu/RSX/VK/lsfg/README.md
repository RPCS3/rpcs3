# Lossless Scaling's frame generation (PS5)

The compute chain of Lossless Scaling's frame generation (LSFG), as the Simpsons
Game's PS5 port runs it on the console, behind the PS5 port's frame generation
setting (`../VKFrameGenLSFG.h`, Frame Generation Method: Lossless Scaling).

## Where it comes from

- [lsfg-vk](https://github.com/PancakeTAS/lsfg-vk) (2025), the Linux Vulkan layer
  that runs Lossless Scaling's frame generation;
- [Eden](https://github.com/eden-emulator/mirror/tree/5f142c7926d0c7fcbbd0ce30794d72f638a43b2a),
  which took it into its Vulkan presenter (revision `5f142c79`);
- the Simpsons Game PS5 builder
  ([KongaTime/TheSimpsonsGame-PS5-Builder](https://github.com/KongaTime/TheSimpsonsGame-PS5-Builder),
  `59e7086`), whose ReXGlue fork adapted Eden's to the console
  (`tools/rexglue-sdk/src/ui/vulkan/framegen/`), with its frame-generation work
  credited to [gyn7561](https://github.com/gyn7561) through
  [ProsperoEden](https://github.com/blackbearreloaded/ProsperoEden).

Every file here but `device_shim.h`, `lsfg_frame_generator.cpp`, `spirv.h` and
this one is that folder's, as it was, except:

- `vulkan_adapter.h` includes `device_shim.h`, RPCS3's device in ReXGlue's
  `VulkanDevice`'s shape, for ReXGlue's own;
- `lsfg_shaders.cpp` has no SPIRV-Tools validation (RPCS3 does not build
  SPIRV-Tools); its own checks of each module stay;
- `lossless_dll.*` and `lsfg_shaders.*` take a path as a `std::string`, not a
  `std::filesystem::path`;
- `frame_gen_pacer.*` is left out: RPCS3's present path decides how many frames
  to make between (`VKPresent.cpp`).

`lsfg_frame_generator.cpp` is the port's `vulkan_framegen_prospero.cpp` reworked
for RPCS3: the game's frame copied into the chain, the frames between written
into RPCS3 images.

## The shaders

They are not here. Lossless Scaling's `Lossless.dll` carries them as SPIR-V
resources; `lossless_dll.cpp` reads them from the player's own copy (bought on
Steam), never loading or running the DLL, from `rpcs3/lossless/Lossless.dll` in
the title's folder (or `rpcs3/Lossless.dll`).

## Licences

- The chain: GPL-3.0-or-later (`COPYING`), as Eden's and lsfg-vk's; the files
  keep their SPDX headers. RPCS3 is GPL-2.0-only, and the two cannot be combined
  in a program for distribution: the fork's README already says not to share
  builds of this port, and this is one more reason.
- `spirv.h`: SPIR-V Headers, the Khronos Group, MIT (`LICENSE.spirv-headers`).
- `Lossless.dll`: proprietary, the player's own, never part of the build.
