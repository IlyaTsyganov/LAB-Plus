"""Regenerate the committed SPIR-V header with glslangValidator or glslc.

The runtime/build does not require a shader compiler. Use this after editing
shaders/collision.vert or collision.frag; pass --compiler if it is not on PATH.
"""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--compiler", help="Path to glslangValidator or glslc")
args = parser.parse_args()
compiler = args.compiler or shutil.which("glslangValidator") or shutil.which("glslc")
if not compiler:
    parser.error("Provide glslangValidator/glslc from a Vulkan SDK with --compiler.")
root = Path(__file__).resolve().parent.parent
header = ["#pragma once", "#include <cstdint>", "// Generated from shaders/collision.vert and collision.frag."]
with tempfile.TemporaryDirectory(prefix="labplus-shaders-") as temporary:
    for extension, name in (("vert", "kVertexShader"), ("frag", "kFragmentShader")):
        source = root / "shaders" / ("collision." + extension)
        target = Path(temporary) / (extension + ".spv")
        command = [compiler]
        if "glslang" in Path(compiler).name.lower():
            command += ["-V"]
        command += [str(source), "-o", str(target)]
        subprocess.run(command, check=True)
        binary = target.read_bytes()
        if len(binary) % 4 or binary[:4] != b"\x03\x02\x23\x07":
            raise ValueError("Compiler did not produce SPIR-V: " + str(source))
        words = struct.unpack("<" + "I" * (len(binary) // 4), binary)
        header.append("static constexpr uint32_t " + name + "[] = {")
        header.extend("    " + ", ".join(f"0x{word:08x}" for word in words[i:i + 8]) + "," for i in range(0, len(words), 8))
        header.append("};")
(root / "src" / "shaders_spirv.h").write_text("\n".join(header) + "\n", encoding="utf-8")
print("Updated src/shaders_spirv.h")
