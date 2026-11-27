"""Read-only audit of user-supplied retail EXEs (requires pefile).

The local manifest contains name/path/timestamp/imageSize/construct/register/
setter and their audited code prefixes. EXEs and local paths are never copied
into the output. Actual registration is covered by LabCvarTests and live probes;
this script checks the retail metadata consumed by the shared implementation.
"""
import argparse
import json
import re
import struct
from pathlib import Path

import pefile


def reflected_structs(pe, kind):
    data, base = pe.__data__, pe.OPTIONAL_HEADER.ImageBase
    start = 0
    while True:
        at = data.find(kind.encode() + b"\0", start)
        if at < 0:
            return
        start = at + len(kind) + 1
        address = struct.pack("<Q", base + pe.get_rva_from_offset(at))
        for ref in re.finditer(re.escape(address), data):
            try:
                size = struct.unpack_from("<I", data, ref.start() + 16)[0]
                if not 0 < size <= 0x1000000:
                    continue
                fields = pe.get_offset_from_rva(struct.unpack_from("<Q", data, ref.start() + 32)[0] - base)
                result = {}
                for index in range(4096):
                    field = fields + index * 72
                    pointer = struct.unpack_from("<Q", data, field + 16)[0]
                    if not pointer:
                        if result:
                            yield size, result
                        break
                    name = pe.get_offset_from_rva(pointer - base)
                    end = data.find(b"\0", name, name + 256)
                    if end < 0:
                        break
                    offset, length = struct.unpack_from("<II", data, field + 24)
                    if length and offset + length <= size:
                        result[data[name:end].decode()] = [offset, length]
            except (ValueError, struct.error, UnicodeDecodeError, pefile.PEFormatError):
                continue


def reflected_flags(pe):
    data, base = pe.__data__, pe.OPTIONAL_HEADER.ImageBase
    at = data.find(b"cvarFlags_t\0")
    if at < 0:
        return {}
    address = struct.pack("<Q", base + pe.get_rva_from_offset(at))
    for ref in re.finditer(re.escape(address), data):
        try:
            entries = pe.get_offset_from_rva(struct.unpack_from("<Q", data, ref.start() + 24)[0] - base)
            values = {}
            for index in range(4096):
                pointer, value = struct.unpack_from("<QQ", data, entries + index * 16)
                if not pointer:
                    break
                name = pe.get_offset_from_rva(pointer - base)
                end = data.find(b"\0", name, name + 128)
                if end < 0:
                    break
                values[data[name:end].decode()] = value
            if "CVAR_BOOL" in values and "CVAR_NOCHEAT" in values:
                return {key: values.get(key) for key in ("CVAR_BOOL", "CVAR_INTEGER", "CVAR_NOCHEAT")}
        except (ValueError, struct.error, UnicodeDecodeError, pefile.PEFormatError):
            continue
    return {}


def require(condition, name, detail):
    if not condition:
        raise RuntimeError(f"{name}: {detail}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    profile_source = (root / "Native/CollisionViewer/src/native_profile.h").read_text()
    expected = {(int(timestamp, 16), int(size, 16)) for _, timestamp, size in re.findall(
        r'"DOOM Eternal ([^"]+)",\s*(0x[0-9a-f]+),\s*(0x[0-9a-f]+)', profile_source)}
    rows = json.loads(args.manifest.read_text())
    identities = {(row["timestamp"], row["imageSize"]) for row in rows}
    require(len(rows) == len(identities) == len(expected) and identities == expected,
            "manifest", "must include each supported retail identity exactly once")
    fields = {
        "valueString": [0, 8], "valueInteger": [8, 4], "name": [40, 8],
        "resetString": [48, 8], "description": [56, 8], "flags": [64, 4],
        "valueMin": [68, 4], "valueMax": [72, 4], "valueStrings": [80, 8],
        "valueCompletion": [88, 8],
    }
    flags = {"CVAR_BOOL": 1, "CVAR_INTEGER": 2, "CVAR_NOCHEAT": 16}
    value_format = '"%s" is:"%s"^7 default:"%s"\n'
    description_format = '^7%s\n'
    results = []
    for row in rows:
        name = row["name"]
        pe = pefile.PE(row["path"], fast_load=True)
        try:
            require((pe.FILE_HEADER.TimeDateStamp, pe.OPTIONAL_HEADER.SizeOfImage) ==
                    (row["timestamp"], row["imageSize"]), name, "EXE identity differs from manifest")
            require(any(size == 104 and all(actual.get(key) == value for key, value in fields.items())
                        for size, actual in reflected_structs(pe, "idCVar::cvarData_t")), name, "cvar field layout differs")
            globals_ = [actual for _, actual in reflected_structs(pe, "engine_t")
                        if "cvarSystemForTransfer" in actual and "cmdSystemForTransfer" in actual]
            require(globals_ and {actual["cvarSystemForTransfer"][0] - actual["cmdSystemForTransfer"][0]
                                 for actual in globals_} == {-8}, name, "cvar system global differs")
            require(reflected_flags(pe) == flags, name, "cvar type flags differ")
            for address, prefix in (("construct", "constructorPrefix"), ("register", "registerPrefix"), ("setter", "setPrefix")):
                expected_code = bytes.fromhex(row[prefix])
                require(pe.get_data(row[address], len(expected_code)) == expected_code, name, f"{address} ABI changed")
            require(pe.__data__.find(value_format.encode() + b"\0") >= 0, name, "current/default format differs")
            require(pe.__data__.find(description_format.encode() + b"\0") >= 0, name, "description format differs")
            results.append({"name": name, "timestamp": row["timestamp"], "imageSize": row["imageSize"], "passed": True})
            print(f"PASS: {name}: fields, defaults, metadata, flags, factory signatures and two-line format", flush=True)
        finally:
            pe.close()
    report = {
        "scope": "All retail EXEs inspected as data; no executable code loaded or called and no game files modified. This is not gameplay verification.",
        "auditedImages": len(results), "cvarDataSize": 104, "cvarGlobalDelta": -8,
        "fields": fields, "flags": flags,
        "valueFormat": value_format, "descriptionFormat": description_format,
        "profiles": results,
    }
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: all {len(results)} supported retail images match the shared cvar query/default/metadata implementation.")


if __name__ == "__main__":
    main()
