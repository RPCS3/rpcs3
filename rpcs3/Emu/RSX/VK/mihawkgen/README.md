# MihawkGen's frame generation (PS5)

The frame generator of PS5_MihawkGen, behind the PS5 port's frame generation
setting (`../VKFrameGenMihawk.h`, Frame Generation Method: MihawkGen): a
trained interpolation network of 432,308 parameters as compute shaders, reading
the game's last frame and this one and writing the frames between, from the
pictures alone.

## Where it comes from

[KongaTime/PS5_MihawkGen](https://github.com/KongaTime/PS5_MihawkGen), a fork of
Mihawk's PS5_MihawkGen, at `bb45cf1c536996e0bfc295ca91a2896e54d2c44f`:

- `framegen.h`: its `framegen/include/framegen.h`;
- `fg_common.cpp`, `fg_neural.*`, `fg_splat.*`, `fg_vulkan.cpp`: its `framegen/src/`;
- `shaders/*.comp.h`: its `framegen/src/shaders/`, the SPIR-V its
  `framegen/shaders/*.comp` compile to (the 21 recompiled with glslangValidator
  15.1.0 are identical);
- `network.fgw`: its `framegen/models/network.fgw`, the trained weights (1.7 MB),
  compiled into the title by `../VKFrameGenMihawk.cpp`.

Every file is as it was there. RPCS3's integration is `../VKFrameGenMihawk.*`.

## Licence

GPL-3.0-or-later (`COPYING`), as PS5_MihawkGen's `framegen/`; the files keep
their SPDX headers. RPCS3 is GPL-2.0-only, and the two cannot be combined in a
program for distribution: as with `../lsfg/`, builds of this port are not to
be shared.
