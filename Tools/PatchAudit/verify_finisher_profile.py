"""Read-only verification of the 5.1 finisher instruction guards against a local EXE.

No game bytes are exported; output contains identities and verification results.
"""
import argparse
import hashlib
from pathlib import Path
import re
from audit_executables import Image


def verify(path):
    image = Image(path)
    if (image.timestamp, image.size) != (0x607e69b0, 0x1b5dd000):
        raise ValueError('Expected the audited DOOM Eternal 5.1 executable')
    header = (Path(__file__).resolve().parents[2] /
              'Native/CollisionViewer/src/sync_patch_layout.h').read_text(encoding='utf-8')
    block = header.split('kSyncCode51[]{', 1)[1].split('};', 1)[0]
    guards = re.findall(r'\{(0x[0-9a-f]+),(\d+),(0x[0-9a-f]+)ull\}', block)
    if len(guards) < 7:
        raise ValueError('Missing finisher code guards')
    for rva, count, expected in guards:
        at = image.offset(image.base + int(rva, 16))
        if at is None:
            raise ValueError(f'Unmapped instruction guard {rva}')
        value = 1469598103934665603
        for byte in image.data[at:at+int(count)]:
            value = ((value ^ byte) * 1099511628211) & ((1 << 64)-1)
        if value != int(expected, 16):
            raise ValueError(f'Changed finisher instruction guard {rva}')
    print(f'PASS: 5.1 identity and {len(guards)} finisher instruction guards; '
          f'SHA256 {hashlib.sha256(image.data).hexdigest()}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    verify(parser.parse_args().exe)
