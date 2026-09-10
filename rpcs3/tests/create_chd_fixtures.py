"""Generate compressed fixtures for the ISO/CHD tests (requires chdman on PATH)."""
import argparse
from pathlib import Path
import struct
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
sector = 2048
image = bytearray(25 * sector)  # Ends halfway through a 4096-byte CHD hunk.
struct.pack_into('>I', image, 0, 1)
struct.pack_into('>I', image, 12, 24)


def both(offset, value, width):
    image[offset:offset + width * 2] = value.to_bytes(width, 'little') + value.to_bytes(width, 'big')


def record(offset, name, start, size, flags):
    length = 33 + len(name) + (len(name) % 2 == 0)
    image[offset] = length
    both(offset + 2, start, 4)
    both(offset + 10, size, 4)
    image[offset + 18:offset + 25] = bytes([126, 9, 4, 0, 0, 0, 0])
    image[offset + 25] = flags
    both(offset + 28, 1, 2)
    image[offset + 32] = len(name)
    image[offset + 33:offset + 33 + len(name)] = name
    return length


pvd = 16 * sector
image[pvd:pvd + 7] = b'\x01CD001\x01'
both(pvd + 80, 25, 4)
both(pvd + 120, 1, 2)
both(pvd + 124, 1, 2)
both(pvd + 128, sector, 2)
record(pvd + 156, b'\0', 18, sector, 2)
image[17 * sector:17 * sector + 7] = b'\xffCD001\x01'
offset = 18 * sector
offset += record(offset, b'\0', 18, sector, 2)
offset += record(offset, b'\1', 18, sector, 2)
offset += record(offset, b'TEST.BIN;1', 20, 3, 0)
offset += record(offset, b'MULTI.BIN;1', 22, sector, 128)
record(offset, b'MULTI.BIN;1', 24, sector, 0)
image[20 * sector:20 * sector + 3] = b'abc'
for i in range(22 * sector, len(image)):
    image[i] = (i * 17 + i // 251) % 256
# Use createraw so older chdman versions without createdvd can generate these
# 2048-byte-unit images. Retail createdvd outputs can also be tested as pairs.
profiles = [('default', 'lzma,zlib,huff,flac'), ('lzma', 'lzma'),
            ('zlib', 'zlib'), ('huff', 'huff'), ('flac', 'flac')]
for name, codecs in profiles:
    for hunk in (4096, 131072):
        stem = args.output / f'{name}-{hunk}'
        iso = stem.with_suffix('.iso')
        chd = stem.with_suffix('.chd')
        # Refuse to replace existing files in the destination directory.
        with iso.open('xb') as out:
            out.write(image)
        subprocess.run(['chdman', 'createraw', '-i', str(iso), '-o', str(chd),
                        '-hs', str(hunk), '-us', '2048', '-c', codecs, '-np', '2'],
                       check=True)
