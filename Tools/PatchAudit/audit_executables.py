"""Read-only EXE inventory and optional production-decoder audit.

python audit_executables.py --out audit --probe RetailPatchPortProbe.exe GAME.exe ...
Only hashes, identities, metadata RVAs and probe results are written. No retail
instruction arrays, memory dumps or game binaries are copied into the output.
The optional numpy package speeds up metadata indexing; it is not required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


class Image:
    def __init__(self, path):
        self.path = Path(path).resolve()
        self.data = self.path.read_bytes()
        if self.data[:2] != b'MZ':
            raise ValueError('Not a PE file')
        pe = struct.unpack_from('<I', self.data, 60)[0]
        if self.data[pe:pe+4] != b'PE\0\0':
            raise ValueError('Invalid PE signature')
        optional = pe + 24
        if struct.unpack_from('<H', self.data, optional)[0] != 0x20b:
            raise ValueError('Expected Windows x64 PE')
        self.base = struct.unpack_from('<Q', self.data, optional+24)[0]
        self.size = struct.unpack_from('<I', self.data, optional+56)[0]
        self.timestamp = struct.unpack_from('<I', self.data, pe+8)[0]
        self.sections = []
        for i in range(struct.unpack_from('<H', self.data, pe+6)[0]):
            at = optional + struct.unpack_from('<H', self.data, pe+20)[0] + i*40
            virtual, rva, size, raw = struct.unpack_from('<IIII', self.data, at+8)
            if raw+size > len(self.data) or rva+max(size, virtual) > self.size:
                raise ValueError('Invalid PE section bounds')
            self.sections.append((rva, size, raw))

    def offset(self, va):
        for rva, size, raw in self.sections:
            if self.base+rva <= va < self.base+rva+size:
                return raw+va-self.base-rva

    def rva(self, offset):
        for rva, size, raw in self.sections:
            if raw <= offset < raw+size:
                return rva+offset-raw

    def text(self, va):
        offset = self.offset(va)
        if offset is None:
            return None
        end = self.data.find(b'\0', offset, offset+1024)
        if end < 0:
            return None
        try:
            return self.data[offset:end].decode('ascii')
        except UnicodeDecodeError:
            return None

    def metadata(self):
        # Candidate identification only. The C++ production reader independently
        # validates reflected names, bounds, types, ancestry and dependencies.
        a = memoryview(self.data)[:len(self.data)//8*8].cast('Q')
        end = self.base+self.size
        try:
            import numpy as np
            values = np.frombuffer(self.data[:len(a)*8], dtype='<u8')
            candidates = np.flatnonzero((values[:-5] >= self.base) & (values[:-5] < end) &
                (values[1:-4] >= self.base) & (values[1:-4] < end) &
                (values[2:-3] > 0) & (values[2:-3] <= 0x200000) &
                ((values[4:-1] == 0) | ((values[4:-1] >= self.base) & (values[4:-1] < end))))
        except ImportError:
            candidates = (n for n in range(len(a)-5) if self.base <= a[n] < end and
                self.base <= a[n+1] < end and 0 < a[n+2] <= 0x200000 and
                (a[n+4] == 0 or self.base <= a[n+4] < end))
        found, scores = {}, {}
        for n in candidates:
            n = int(n)
            name, parent, rva = self.text(a[n]), self.text(a[n+1]), self.rva(n*8)
            if not name or parent is None or rva is None or len(name) > 255 or not name[0].isalpha():
                continue
            if not all(c.isalnum() or c in '_:<> *,[]' for c in name):
                continue
            rows, score = a[n+4], 0
            for i in range(2000):
                at = self.offset(rows+i*72) if rows else None
                if at is None or at+32 > len(self.data):
                    break
                typ, ops, key, _, _ = struct.unpack_from('<QQQii', self.data, at)
                if not key:
                    break
                if self.text(typ) is None or not self.text(key) or (ops and self.text(ops) is None):
                    score = -1
                    break
                score += 1
            if score < 0:
                continue
            rank = (score, a[n+3] == 0)
            if name not in scores or rank > scores[name]:
                found[name], scores[name] = rva, rank
        return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executables', nargs='+', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--probe', type=Path)
    parser.add_argument('--probe-arg', action='append', default=[], help='Extra argument passed to the audit probe')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    results = []
    failed = False
    for index, path in enumerate(args.executables):
        try:
            image = Image(path)
            identity = hashlib.sha256(image.data).hexdigest()
            stem = f'{index:02d}-{identity[:12]}'
            metadata = args.out / (stem+'.tsv')
            fields = image.metadata()
            metadata.write_text(''.join(f'{name}\t{hex(rva)}\n' for name, rva in sorted(fields.items())), encoding='utf-8')
            result = dict(path=str(image.path), sha256=identity, timestamp=hex(image.timestamp),
                image_size=hex(image.size), metadata_rvas=metadata.name, metadata_candidates=len(fields), state='not_run')
            if args.probe:
                process = subprocess.run([str(args.probe.resolve()), str(image.path), str(metadata.resolve()), *args.probe_arg],
                    capture_output=True, timeout=120)
                log = process.stdout.decode(errors='replace')+process.stderr.decode(errors='replace')
                (args.out / (stem+'.log')).write_text(log, encoding='utf-8')
                result.update(state='passed' if process.returncode == 0 else 'failed', exit_code=process.returncode)
                failed |= process.returncode != 0
            results.append(result)
            print(f'{image.path.parent.name}: {result["state"]}; {identity}', flush=True)
        except (OSError, ValueError, struct.error, subprocess.TimeoutExpired) as error:
            results.append(dict(path=str(path), state='error', error=str(error)))
            failed = True
    (args.out / 'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
