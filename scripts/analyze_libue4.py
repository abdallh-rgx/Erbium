#!/usr/bin/env python3
"""
analyze_libue4.py — offline ELF analyzer for Fortnite Android's libUE4.so /
libUnreal.so. Produces a baked-offsets header consumed by the Android port,
replacing upstream's runtime x64 pattern scanning (Finders.cpp) with
per-version constants — the same philosophy as Owen.c's offsets table.

What it extracts (phase by phase):
  phase 1 (now):    ELF sanity, arch check, symbol table dump, version string,
                    UTF-16 string anchors, section map → offsets.h skeleton
  phase 2 (next):   GObjects / GNames / GWorld candidates via reference anchors
  phase 3:          server-side function offsets (TickFlush, encryption patch,
                    GIsClient/GIsServer globals, null/ret-true targets)

Usage:
  python3 scripts/analyze_libue4.py --apk fortnite.apk --version 18.40.0-18167774 --out android/Generated/offsets.h
  python3 scripts/analyze_libue4.py --so libUE4.so --version 18.40.0-18167774 --out android/Generated/offsets.h
"""
import argparse
import io
import json
import os
import re
import struct
import sys
import zipfile

ELF_MAGIC = b"\x7fELF"


def extract_lib_from_apk(apk_path):
    """Return (name, bytes) of the UE4/Unreal shared lib inside the APK/XAPK."""
    zf = zipfile.ZipFile(apk_path)
    candidates = []
    for n in zf.namelist():
        base = os.path.basename(n)
        if base in ("libUE4.so", "libUnreal.so") and "arm64-v8a" in n:
            candidates.append((n, base))
    if not candidates:
        # any arm64 lib that looks like the engine
        for n in zf.namelist():
            if "arm64-v8a" in n and n.endswith(".so"):
                candidates.append((n, os.path.basename(n)))
    if not candidates:
        raise RuntimeError("no arm64 native lib found in APK")
    # prefer libUE4.so
    candidates.sort(key=lambda c: 0 if c[1] == "libUE4.so" else 1)
    name, base = candidates[0]
    print(f"[analyze] extracting {name} ({zf.getinfo(name).file_size/1024/1024:.1f} MB)")
    return base, zf.read(name)


class Elf64:
    """Minimal ELF64 little-endian parser."""

    def __init__(self, data: bytes):
        self.data = data
        if data[:4] != ELF_MAGIC:
            raise RuntimeError("not an ELF")
        if data[4] != 2 or data[5] != 1:
            raise RuntimeError("not ELF64 LE")
        (self.e_type, self.e_machine, _ver, self.e_entry, self.e_phoff,
         self.e_shoff, self.e_flags, self.e_ehsize, self.e_phentsize,
         self.e_phnum, self.e_shentsize, self.e_shnum,
         self.e_shstrndx) = struct.unpack_from("<HHIQQQIHHHHHH", data, 16)
        self.sections = []
        for i in range(self.e_shnum):
            off = self.e_shoff + i * self.e_shentsize
            (sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size,
             sh_link, sh_info, sh_addralign, sh_entsize) = struct.unpack_from(
                "<IIQQQQIIQQ", data, off)
            self.sections.append(dict(name_off=sh_name, type=sh_type, flags=sh_flags,
                                      addr=sh_addr, offset=sh_offset, size=sh_size,
                                      link=sh_link, entsize=sh_entsize))
        if self.e_shstrndx < len(self.sections):
            strtab = self.sections[self.e_shstrndx]
            self._strtab = data[strtab["offset"]:strtab["offset"] + strtab["size"]]
        else:
            self._strtab = b""

    def sh_name(self, sec):
        end = self._strtab.find(b"\x00", sec["name_off"])
        return self._strtab[sec["name_off"]:end].decode(errors="ignore")

    def section(self, name):
        for s in self.sections:
            if self.sh_name(s) == name:
                return s
        return None

    def section_bytes(self, name):
        s = self.section(name)
        return self.data[s["offset"]:s["offset"] + s["size"]] if s else b""

    def find_string_offset(self, s, needle: bytes):
        """Return file/vaddr offset of first occurrence of needle in section."""
        blob = self.data[s["offset"]:s["offset"] + s["size"]]
        i = blob.find(needle)
        if i < 0:
            return None
        return s["addr"] + i, s["offset"] + i  # (vaddr, fileoff)

    def symbols(self):
        """Yield (name, value, size, defined) from .dynsym and .symtab if present."""
        out = []
        for tab, strt in ((".dynsym", ".dynstr"), (".symtab", ".strtab")):
            symsec = self.section(tab)
            if not symsec:
                continue
            strsec = self.section(strt)
            strblob = self.data[strsec["offset"]:strsec["offset"] + strsec["size"]] if strsec else b""
            blob = self.data[symsec["offset"]:symsec["offset"] + symsec["size"]]
            count = len(blob) // 24
            for i in range(count):
                st_name, st_info, st_other, st_shndx, st_value, st_size = struct.unpack_from(
                    "<IBBHQQ", blob, i * 24)
                if st_name == 0:
                    continue
                end = strblob.find(b"\x00", st_name)
                name = strblob[st_name:end].decode(errors="ignore")
                if name:
                    out.append((name, st_value, st_size, tab))
        return out


def find_utf16(data: bytes, text: str):
    """Find a UTF-16LE encoded string in a buffer; return offset or None."""
    needle = text.encode("utf-16-le") + b"\x00\x00"
    return data.find(needle)


def analyze(so_name: str, blob: bytes, version: str, out_path: str):
    print(f"[analyze] parsing {so_name} — {len(blob)/1024/1024:.1f} MB")
    elf = Elf64(blob)

    mach = {183: "aarch64", 40: "arm", 62: "x86_64", 3: "x86"}.get(elf.e_machine, str(elf.e_machine))
    print(f"[analyze] machine: {mach}  type: {elf.e_type}  sections: {elf.e_shnum}")
    if mach != "aarch64":
        print("[analyze] WARNING: expected aarch64!")

    report = {
        "lib": so_name,
        "version": version,
        "machine": mach,
        "sections": {elf.sh_name(s): hex(s["addr"]) for s in elf.sections if s["type"] != 0},
    }

    # 1) game version string anchor ("++Fortnite+Release-...-CL-...")
    m = re.search(rb"\+\+Fortnite\+Release-[\d.]+-CL-\d+", blob)
    report["release_string"] = m.group(0).decode() if m else None
    print(f"[analyze] release string: {report['release_string']}")

    # 2) symbol tables
    syms = elf.symbols()
    report["symbol_count"] = len(syms)
    print(f"[analyze] symbols: {len(syms)}")
    interesting = [s for s in syms if re.search(
        r"curl|GObjects|GNames|GWorld|ProcessEvent|TickFlush|Beacon|NetDriver", s[0], re.I)]
    report["interesting_symbols"] = [
        {"name": n, "addr": hex(v), "size": sz, "tab": t} for (n, v, sz, t) in interesting[:200]]

    # 3) key UTF-16 string anchors used by upstream Finders.cpp
    anchors = {}
    for ro_name in (".rodata", ".data.rel.ro", ".data"):
        s = elf.section(ro_name)
        if not s:
            continue
        for text in ("AllowCommandletRendering", "Athena_Terrain", "Artemis_Terrain",
                     "/Game/Athena/Playlists", "LogFortUIManager"):
            idx = find_utf16(blob[s["offset"]:s["offset"] + s["size"]], text)
            if idx is not None:
                anchors[text] = hex(s["addr"] + idx)
    report["utf16_anchors"] = anchors
    print(f"[analyze] utf16 anchors: {anchors}")

    # 4) write the baked offsets header
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w") as f:
        f.write("// GENERATED by scripts/analyze_libue4.py — DO NOT EDIT\n")
        f.write(f"// lib: {so_name}  version: {version}\n")
        f.write(f"// release: {report.get('release_string')}\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("namespace Erbium::Baked {\n")
        f.write(f"inline constexpr const char* kLibName = \"{so_name}\";\n")
        f.write(f"inline constexpr const char* kRelease = \"{report.get('release_string') or version}\";\n")
        for text, addr in anchors.items():
            ident = re.sub(r"\W+", "_", text)
            f.write(f"inline constexpr uint64_t kStr_{ident} = {addr}; // \"{text}\"\n")
        for sym in interesting:
            ident = re.sub(r"\W+", "_", sym[0])
            f.write(f"inline constexpr uint64_t kSym_{ident} = {hex(sym[1])}; // {sym[0]} ({sym[3]})\n")
        f.write("} // namespace Erbium::Baked\n")
    print(f"[analyze] wrote {out_path}")

    # 5) sidecar json next to the header
    with open(os.path.splitext(out_path)[0] + ".json", "w") as f:
        json.dump(report, f, indent=1)
    print("[analyze] phase-1 analysis complete")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apk")
    ap.add_argument("--so")
    ap.add_argument("--version", default="unknown")
    ap.add_argument("--out", default="android/Generated/offsets.h")
    args = ap.parse_args()

    if args.so:
        name = os.path.basename(args.so)
        with open(args.so, "rb") as f:
            blob = f.read()
    elif args.apk:
        name, blob = extract_lib_from_apk(args.apk)
    else:
        print("need --apk or --so")
        return 2

    analyze(name, blob, args.version, args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
