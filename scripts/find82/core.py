#!/usr/bin/env python3
"""find82 core — ARM64 static analysis primitives for locating Erbium's 82
functions inside libUnreal.so (21.30.0-CL-21088273).

Primitives:
  Elf64 (from analyze_libue4) + APS2 relocations → got_map / relro_map
  xref(addr)        — all ADRP+ADD / ADRP+LDR instruction PCs targeting addr
  xref_str(s)       — xrefs to the UTF-16 (or ASCII) string s
  func_start(pc)    — start of the function containing pc (BL-target table)
  func_calls(f)     — BL callees of function f
  callers(f)        — BL callers of function f
  vtables           — {vtable_base: {slot: func}} from .data.rel.ro relocs
  vtables_of(func)  — vtables containing func (and its slot index)
  disasm(pc, n)     — capstone disassembly
"""
import array
import re
import struct
import sys
from collections import defaultdict

sys.setrecursionlimit(10000)

# --- ELF -------------------------------------------------------------------
class Elf64:
    def __init__(self, blob):
        self.b = blob
        assert blob[:4] == b"\x7fELF" and blob[4] == 2
        (self.e_type, self.e_machine, _, self.e_entry, self.e_phoff, self.e_shoff,
         self.e_flags, self.e_ehsize, self.e_phentsize, self.e_phnum,
         self.e_shentsize, self.e_shnum, self.e_shstrndx) = struct.unpack_from("<HHIQQQIHHHHHH", blob, 16)
        self.sections = []
        for i in range(self.e_shnum):
            off = self.e_shoff + i * self.e_shentsize
            name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from("<IIQQQQIIQQ", blob, off)
            self.sections.append(dict(name_off=name, type=typ, addr=addr, offset=offset, size=size, link=link, entsize=entsize))
        shstr = self.sections[self.e_shstrndx]
        def cstr(o):
            e = blob.index(b"\0", shstr["offset"] + o)
            return blob[shstr["offset"] + o:e].decode()
        for s in self.sections:
            s["sname"] = cstr(s["name_off"]) if s["name_off"] < shstr["size"] else ""
        self._by_name = {s["sname"]: s for s in self.sections}

    def section(self, n):
        return self._by_name.get(n)

    def addr_to_off(self, addr):
        for s in self.sections:
            if s["type"] != 8 and s["addr"] <= addr < s["addr"] + s["size"]:
                return s["offset"] + (addr - s["addr"])
        return None

    def read(self, addr, n):
        o = self.addr_to_off(addr)
        return self.b[o:o + n] if o is not None else None

    def dynsyms(self):
        dynsym, dynstr = self.section(".dynsym"), self.section(".dynstr")
        out = []
        for i in range(dynsym["size"] // 24):
            off = dynsym["offset"] + i * 24
            nm, info, other, shndx, value, size = struct.unpack_from("<IBBHQQ", self.b, off)
            end = self.b.index(b"\0", dynstr["offset"] + nm)
            name = self.b[dynstr["offset"] + nm:end].decode()
            out.append((name, value, size))
        return out

    def find_utf16(self, s, sec=None):
        # NOTE: UE packs string literals with SINGLE NUL terminators — do not
        # require a trailing double NUL.
        pat = s.encode("utf-16-le")
        if sec:
            blob = self.b[sec["offset"]:sec["offset"] + sec["size"]]
            i = blob.find(pat)
            return sec["addr"] + i if i >= 0 else None
        i = self.b.find(pat)
        return i if i >= 0 else None  # addr==off in this link layout for loaded sections

    def find_ascii(self, s, sec=None):
        pat = s.encode()
        blob = self.b if not sec else self.b[sec["offset"]:sec["offset"] + sec["size"]]
        i = blob.find(pat)
        return i if i >= 0 else None


# --- APS2 packed relocations (Android-10 bionic semantics) -----------------
REL_BY_INFO = 1
REL_BY_OFFSET_DELTA = 2
REL_GROUPED_BY_ADDEND = 4
REL_HAS_ADDEND = 8
R_AARCH64_RELATIVE = 1027

class Sleb:
    def __init__(self, b, pos):
        self.b, self.pos = b, pos
    def sleb(self):
        r = s = 0
        while True:
            byte = self.b[self.pos]; self.pos += 1
            r |= (byte & 0x7F) << s
            s += 7
            if not (byte & 0x80):
                if byte & 0x40:
                    r -= 1 << s
                return r

def decode_aps2(elf):
    """→ (got_map {slot_addr: addend}, relro_map {addr: addend})"""
    got_map, relro_map = {}, {}
    rela = elf.section(".rela.dyn")
    got = elf.section(".got") or elf.section(".got.plt")
    rro = elf.section(".data.rel.ro")
    got_lo, got_hi = got["addr"], got["addr"] + got["size"]
    rro_lo = rro["addr"] if rro else 0
    rro_hi = rro["addr"] + rro["size"] if rro else 0
    d = Sleb(elf.b, rela["offset"] + 4)
    count = d.sleb(); r_offset = d.sleb(); r_info = 0; r_addend = 0
    idx = 0
    while idx < count:
        gs = d.sleb(); gf = d.sleb(); grod = 0
        if gf & REL_BY_OFFSET_DELTA:
            grod = d.sleb()
        if gf & REL_BY_INFO:
            r_info = d.sleb()
        if (gf & REL_HAS_ADDEND) and (gf & REL_GROUPED_BY_ADDEND):
            r_addend += d.sleb()
        elif not (gf & REL_HAS_ADDEND):
            r_addend = 0
        for _ in range(gs):
            if gf & REL_BY_OFFSET_DELTA:
                r_offset += grod
            else:
                r_offset += d.sleb()
            if not (gf & REL_BY_INFO):
                r_info = d.sleb()
            if (gf & REL_HAS_ADDEND) and not (gf & REL_GROUPED_BY_ADDEND):
                r_addend += d.sleb()
            if (r_info & 0xFFFFFFFF) == R_AARCH64_RELATIVE:
                if got_lo <= r_offset < got_hi:
                    got_map[r_offset] = r_addend
                elif rro_lo <= r_offset < rro_hi:
                    relro_map[r_offset] = r_addend
            idx += 1
    return got_map, relro_map


# --- ARM64 instruction decoders ---------------------------------------------
def adrp_decode(pc, w):
    if (w & 0x9F000000) != 0x90000000:
        return None
    rd = w & 31
    immlo = (w >> 29) & 3
    immhi = (w >> 5) & 0x7FFFF
    imm = (immhi << 2) | immlo
    if imm & 0x100000:
        imm -= 0x200000
    return rd, (pc & ~0xFFF) + (imm << 12), rd

def add_imm_decode(w):
    if (w & 0x7F800000) != 0x11000000 and (w & 0xFF800000) != 0x91000000:
        return None
    rd = w & 31; rn = (w >> 5) & 31
    imm = (w >> 10) & 0xFFF
    sh = (w >> 22) & 3
    if sh == 1:
        imm <<= 12
    return rd, rn, imm

def ldrx_imm_decode(w):
    if (w & 0xFFC00000) != 0xF9400000:
        return None
    rt = w & 31; rn = (w >> 5) & 31; imm = ((w >> 10) & 0xFFF) * 8
    return rt, rn, imm

def ldr_lit_decode(pc, w):
    """LDR Xt, label (literal load from PC-relative pool)"""
    if (w & 0xFF000000) != 0x58000000:
        return None
    rt = w & 31
    imm19 = (w >> 5) & 0x7FFFF
    if imm19 & 0x40000:
        imm19 -= 0x80000
    return rt, pc + imm19 * 4

def bl_decode(pc, w):
    if (w & 0xFC000000) != 0x94000000:
        return None
    imm26 = w & 0x3FFFFFF
    if imm26 & 0x2000000:
        imm26 -= 0x4000000
    return pc + imm26 * 4


# --- The analysis engine ----------------------------------------------------
class Lib:
    def __init__(self, path):
        self.path = path
        blob = open(path, "rb").read()
        self.b = blob
        self.elf = elf = Elf64(blob)
        text = elf.section(".text")
        self.TEXT_LO, self.TEXT_SZ = text["addr"], text["size"]
        self.TEXT_HI = self.TEXT_LO + self.TEXT_SZ
        self.text_arr = array.array("I")
        self.text_arr.frombytes(blob[text["offset"]:text["offset"] + text["size"]])
        self.got_map, self.relro_map = decode_aps2(elf)

        # --- one pass over .text: ADRP tracking → full xref map -----------
        self.xrefs = defaultdict(list)      # target_addr -> [pcs]
        self.bl_edges = []                  # (pc, target)
        n = len(self.text_arr)
        pending = {}                        # pc -> (rd, page)
        for i in range(n):
            pc = self.TEXT_LO + i * 4
            w = self.text_arr[i]
            r = adrp_decode(pc, w)
            if r:
                pending[pc] = (r[0], r[1])
                if len(pending) > 64:
                    oldest = min(pending)
                    del pending[oldest]
                continue
            if (w & 0xFC000000) == 0x94000000:
                self.bl_edges.append((pc, bl_decode(pc, w)))
            ad = add_imm_decode(w)
            if ad:
                for ppc in list(pending):
                    rd, page = pending[ppc]
                    if rd == ad[1] and pc - ppc <= 0x2000:
                        self.xrefs[page + ad[2]].append(ppc)
                        del pending[ppc]
                continue
            ld = ldrx_imm_decode(w)
            if ld:
                for ppc in list(pending):
                    rd, page = pending[ppc]
                    if rd == ld[1] and pc - ppc <= 0x2000:
                        addr = page + ld[2]
                        self.xrefs[addr].append(ppc)
                        del pending[ppc]
                continue
            ll = ldr_lit_decode(pc, w)
            if ll:
                self.xrefs[ll[1]].append(pc)

        # --- function starts: BL targets + dynsym + relro targets ----------
        starts = set(t for _, t in self.bl_edges if self.TEXT_LO <= t < self.TEXT_HI)
        for name, value, size in elf.dynsyms():
            if value and self.TEXT_LO <= value < self.TEXT_HI:
                starts.add(value)
        for tgt in list(self.relro_map.values()) + list(self.got_map.values()):
            if tgt and self.TEXT_LO <= tgt < self.TEXT_HI:
                starts.add(tgt)
        self.funcs = sorted(starts)
        self.func_set = set(self.funcs)
        self.func_index = {f: i for i, f in enumerate(self.funcs)}
        # caller map
        self.callers = defaultdict(set)
        self.callees = defaultdict(list)
        for pc, tgt in self.bl_edges:
            f = self.func_start_of(pc)
            if f is not None:
                self.callees[f].append(tgt)
                self.callers[tgt].add(f)

        # --- vtables from relro: contiguous runs of function pointers -----
        self.vtables = []                    # list of (base_addr, {slot: func})
        run_start = None; run = {}
        prev = None
        for addr in sorted(self.relro_map):
            tgt = self.relro_map[addr]
            if tgt in self.func_set or tgt == 0:
                if prev is not None and addr != prev + 8:
                    if len(run) >= 4:
                        self.vtables.append((run_start, dict(run)))
                    run = {}; run_start = None
                if run_start is None:
                    run_start = addr
                run[(addr - run_start) // 8] = tgt
                prev = addr
            else:
                if len(run) >= 4:
                    self.vtables.append((run_start, dict(run)))
                run = {}; run_start = None; prev = None
        if len(run) >= 4:
            self.vtables.append((run_start, dict(run)))
        self.func_vtables = defaultdict(list)   # func -> [(vbase, slot)]
        for vbase, slots in self.vtables:
            for slot, fn in slots.items():
                if fn:
                    self.func_vtables[fn].append((vbase, slot))

    # --- API ----------------------------------------------------------------
    def func_start_of(self, pc):
        import bisect
        i = bisect.bisect_right(self.funcs, pc) - 1
        if i < 0:
            return None
        return self.funcs[i]

    def func_end(self, f):
        i = self.func_index.get(f)
        if i is None:
            return None
        return self.funcs[i + 1] if i + 1 < len(self.funcs) else self.TEXT_HI

    def xref_str(self, s, wide=True, ascii_too=False):
        out = []
        if wide:
            a = self.elf.find_utf16(s)
            if a is not None:
                out += self.xrefs.get(a, [])
        if ascii_too:
            a = self.elf.find_ascii(s)
            if a is not None:
                out += self.xrefs.get(a, [])
        return sorted(set(out))

    def disasm(self, pc, n=20):
        import capstone
        md = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_LITTLE_ENDIAN)
        off = self.elf.addr_to_off(pc)
        code = self.b[off:off + n * 4]
        return list(md.disasm(code, pc))

    def dump(self, f, max_len=None):
        end = self.func_end(f) if max_len is None else min(self.func_end(f) or 0, f + max_len)
        lines = []
        pc = f
        while pc < end:
            for ins in self.disasm(pc, 1):
                lines.append(f"{ins.address:#x}: {ins.mnemonic:10s} {ins.op_str}")
            pc += 4
        return "\n".join(lines)

    def resolve_adrp_target(self, pc):
        """What address does the ADRP(+ADD/LDR) at pc reference?"""
        w = self.text_arr[(pc - self.TEXT_LO) // 4]
        r = adrp_decode(pc, w)
        if not r:
            return None
        page = r[1]
        i = (pc - self.TEXT_LO) // 4 + 1
        if i < len(self.text_arr):
            w2 = self.text_arr[i]
            ad = add_imm_decode(w2)
            if ad and ad[1] == r[0]:
                return page + ad[2]
            ld = ldrx_imm_decode(w2)
            if ld and ld[1] == r[0]:
                return page + ld[2]
        return page

    def strings_in_func(self, f, limit=200):
        """All .rodata strings referenced by function f."""
        end = self.func_end(f)
        i0 = (f - self.TEXT_LO) // 4
        i1 = (end - self.TEXT_LO) // 4 if end else i0 + 100000
        out = []
        for i in range(i0, min(i1, i0 + 200000)):
            pc = self.TEXT_LO + i * 4
            w = self.text_arr[i]
            if (w & 0x9F000000) == 0x90000000:
                t = self.resolve_adrp_target(pc)
                if t is not None:
                    s = self.read_str(t)
                    if s and len(s) >= 4:
                        out.append(s[:120])
        return out

    def read_str(self, addr, wide=True):
        """Read UTF-16 or ASCII NUL-terminated string at addr."""
        raw = self.elf.read(addr, 512)
        if raw is None:
            return None
        if wide:
            i = 0
            chars = []
            while i + 1 < len(raw) and len(chars) < 200:
                c = raw[i] | (raw[i + 1] << 8)
                if c == 0:
                    return "".join(chars) if chars else None
                chars.append(chr(c))
                i += 2
        i = raw.find(b"\0")
        return raw[:i].decode("utf-8", "replace") if i > 0 else None


if __name__ == "__main__":
    import os
    default = os.environ.get('LIBUNREAL') or 'libUnreal.so'
    lib = Lib(sys.argv[1] if len(sys.argv) > 1 else default)
    print("funcs:", len(lib.funcs), " vtables:", len(lib.vtables))
    print("xref targets:", len(lib.xrefs))
