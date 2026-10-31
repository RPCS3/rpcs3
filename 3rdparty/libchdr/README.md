# libchdr integration

The submodule is pinned to `1d40b6eec062479e300d208e4456e6008566615c`.
This tested post-v0.3.0 revision includes malformed-input fixes, including
`d147f76` (undefined shifts) and `76649d7` (Huffman subtable arena reset).
Updating it requires rechecking both build descriptions and the CHD tests.

The local CMake target and Visual Studio project use RPCS3's existing zlib and
Zstd libraries and libchdr's namespaced LZMA decoder. They avoid adding another
copy of zlib/Zstd or importing libchdr's project-wide build options. Per-hunk CRC
verification is enabled. LOWRAM is disabled; each mounted source owns one map
and decoder, serialized by the source's mutex. The loader bounds image and hunk
sizes before opening the decoder. The map limit is not a total process-memory
limit.

Distribution notices for libchdr, its MAME-derived code, dr_flac and LZMA are in
`bin/licenses/libchdr.txt`, included by the application packaging. The bundled
miniz and Zstd sources in the submodule are not built.
