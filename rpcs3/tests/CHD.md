# CHD reader tests

Build `rpcs3_test` with `BUILD_RPCS3_TESTS=ON`. The `iso_test.*` tests include
synthetic standalone CHDv5 images and raw ISO regressions. No game dumps are
required.

To exercise compressed hunks, generate fixtures in a new directory using Python
3 and `chdman` on PATH:

```sh
python3 rpcs3/tests/create_chd_fixtures.py /tmp/rpcs3-chd-fixtures
RPCS3_CHD_FIXTURES=/tmp/rpcs3-chd-fixtures ./build/bin/rpcs3_test --gtest_filter='iso_test.*'
```

On Windows, set `RPCS3_CHD_FIXTURES` in the test process environment.
With GoogleTest 1.8 (MSVC), the two external-fixture tests have a `DISABLED_`
prefix because runtime skipping is unavailable. Run them explicitly with
`--gtest_filter='iso_test.DISABLED_Chd*' --gtest_also_run_disabled_tests` after
providing fixtures; missing fixtures then fail instead of being reported as passes.

The generator refuses to overwrite existing fixtures. It uses `createraw` with 2048-byte units
for compatibility with older chdman versions without `createdvd`. It produces
LZMA, zlib, Huffman, FLAC and mixed-codec images at 4 KiB and 128 KiB hunk sizes.
On newer GoogleTest versions, two tests skip when their external fixtures are
unavailable; check the test summary before claiming compressed coverage.

`ChdExternalCompressedFixtures` also accepts a directory of `.chd` files paired
with their original `.iso` files under matching stems. It compares the entire
decoded stream, then parses the ISO hierarchy. This can check `createdvd`
output, including Zstd, without adding copyrighted data to the repository.
It is a byte-identity check, not a game-boot or performance test.
