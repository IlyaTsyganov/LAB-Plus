"""Read-only verification of configured checkpoint adapters against retail EXEs.

python Tools/PatchAudit/verify_practice_profiles.py --out build/practice-audit.json GAME.exe ...
Checks exact retail SHA-256, PE identity, loader/string method fingerprints,
engine globals and reflected pending-save/manager fields. No game is launched.
This is static verification, not live save/load acceptance.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from audit_executables import Image

ROOT = Path(__file__).resolve().parents[2]


def profiles():
    native = (ROOT / 'Native/CollisionViewer/src/native_profile.h').read_text(encoding='utf-8-sig')
    games = {key: (int(ts, 16), int(size, 16), int(fingerprint, 16), int(system, 16))
             for key, ts, size, fingerprint, system in re.findall(
                 r'static constexpr GameProfile (kProfile\w+)\{\s*"[^"]+",(0x[0-9a-f]+),(0x[0-9a-f]+),\s*(0x[0-9a-f]+),0x[0-9a-f]+,(0x[0-9a-f]+)', native)}
    text = (ROOT / 'Native/CollisionViewer/src/practice_profile.h').read_text(encoding='utf-8-sig')
    found = {}
    for match in re.finditer(r'// SHA256 ([0-9a-f]{64})\s*static const PracticeProfile \w+\{(.*?)\n\};', text, re.S):
        sha, body = match.groups()
        version, game = re.search(r'"([^"]+)",&(kProfile\w+),', body).groups()
        prefix, signature_text = body.split('{{', 1)
        numbers = [int(v, 0) for v in re.findall(r'0x[0-9a-f]+|(?<![\w])\d+(?![\w])', prefix.split(',', 2)[2])]
        signatures = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-f]{2})', signature_text))
        if len(numbers) != 14 or len(signatures) != 80:
            raise ValueError('Malformed configured practice profile: ' + version)
        ts, size, fingerprint, system_pointer = games[game]
        found[(ts, size)] = dict(version=version, sha256=sha, globals=numbers[:9], methods=numbers[9:],
                                 signatures=signatures, fingerprint=fingerprint, system_pointer=system_pointer)
    if not found:
        raise ValueError('No configured practice profiles')
    return found


def verify(path, configured):
    image = Image(path)
    profile = configured.get((image.timestamp, image.size))
    if profile is None:
        raise ValueError('No configured save/load adapter for this PE identity')
    def read(rva, count):
        offset = image.offset(image.base + rva)
        if offset is None or offset + count > len(image.data):
            raise ValueError('RVA outside file')
        return image.data[offset:offset + count]
    def require(condition, message):
        if not condition:
            raise ValueError(message)
    sha = hashlib.sha256(image.data).hexdigest()
    require(sha == profile['sha256'], 'Retail SHA-256 mismatch')
    require(read(profile['fingerprint'], 10) == bytes.fromhex('48895c2408574883ec20'), 'Game profile fingerprint mismatch')
    for index, method in enumerate(profile['methods']):
        require(read(method, 16) == profile['signatures'][16 * index:16 * index + 16], 'Save/load method fingerprint mismatch')
    system, ghost, users, ghost_offset, pending, slots, map_type, extra, slot_count = profile['globals']
    require(ghost == system + ghost_offset, 'Save manager global does not match static system member')
    require(struct.unpack('<Q', read(profile['system_pointer'], 8))[0] == image.base + system, 'Static engine system pointer mismatch')
    loader = profile['methods'][0]
    require(read(loader, 7) == bytes.fromhex('4883ec28488b05'), 'Unexpected loader prologue')
    require(loader + 11 + struct.unpack('<i', read(loader + 7, 4))[0] == users, 'Master user global mismatch')
    require(struct.unpack('<I', read(loader + 0x27, 4))[0] == ghost_offset, 'Loader save manager member mismatch')
    reflected = []
    for string_match in re.finditer(re.escape(b'idGameSystemLocal\0'), image.data):
        name_rva = image.rva(string_match.start())
        if name_rva is None:
            continue
        pointer = struct.pack('<Q', image.base + name_rva)
        for reference in re.finditer(re.escape(pointer), image.data):
            at = reference.start()
            if at + 40 > len(image.data):
                continue
            parent, class_size, fields = struct.unpack_from('<QI12xQ', image.data, at + 8)
            if image.text(parent) != 'idGameSystem' or not 0 < class_size <= 0x200000:
                continue
            values = {}
            for index in range(2000):
                offset = image.offset(fields + index * 72)
                if offset is None or offset + 32 > len(image.data):
                    break
                key, value = struct.unpack_from('<Qi', image.data, offset + 16)
                if not key:
                    break
                values[image.text(key)] = value
            if values.get('ghostProfileManager') == ghost_offset and values.get('pendingSaveGameRequests', -8) + 8 == pending:
                reflected.append(at)
    require(bool(reflected), 'Reflected save queue/manager fields mismatch')
    expected_slots = 3 if profile['version'] in ('1.0', '1.1', '2.0', '2.1') else 6 if profile['version'] in ('3.0', '3.1', '4.0', '4.1') else 9
    require(slot_count == expected_slots, 'DLC slot availability mismatch')
    matrix = json.loads((ROOT / 'docs/validation/checkpoint-patch-matrix.json').read_text(encoding='utf-8-sig'))
    matching = next((row for row in matrix if row['timestamp'] == hex(image.timestamp)), None)
    require(matching is not None and 'discovered_entry_layout' in matching, 'Discovered-entry audit missing')
    entry = matching['discovered_entry_layout']
    entry_code = read(int(entry['entry_info_getter_rva'], 16), 260)
    require(all(struct.pack('<I', value) in entry_code for value in (0x80, 0x88, 0xe0)), 'Discovered-entry accessor layout mismatch')
    return dict(version=profile['version'], sha256=sha, static_verification='passed', live_acceptance='separate',
                slots=slot_count, timestamp=hex(image.timestamp), image_size=hex(image.size))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executables', nargs='+', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    configured = profiles()
    results = []
    failed = False
    for path in args.executables:
        try:
            result = verify(path, configured)
            print(result['version'] + ': static verification passed; live acceptance is separate', flush=True)
        except (OSError, ValueError, struct.error) as error:
            result = dict(path=str(path), static_verification='failed', error=str(error))
            failed = True
            print(str(path) + ': ' + str(error), flush=True)
        results.append(result)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(results, indent=2), encoding='utf-8')
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
