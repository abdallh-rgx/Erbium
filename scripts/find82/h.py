#!/usr/bin/env python3
"""function-start heuristics for ARM64 + exploration helpers."""
import pickle, bisect, struct
import core, capstone

import os
LIBUNREAL = os.environ.get('LIBUNREAL') or os.path.join(os.path.dirname(__file__), '..', '..', 'artifacts', 'libUnreal.so')
raw = open(LIBUNREAL, 'rb').read()
elf = core.Elf64(raw)
st = pickle.load(open(os.path.join(os.path.dirname(__file__), 'lib_state2.pkl'), 'rb'))
FUNCS = st['funcs']
CALLERS = st['callers']
CALLEES = st['callees']
XREFS = st['xrefs']
FVS = st['func_vtables']
RELRO = st['relro_map']
GOT = st['got_map']
TEXT_LO = 0x2fac000
TEXT_SZ = 0x85ff0a4
TEXT_HI = TEXT_LO + TEXT_SZ

md = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_LITTLE_ENDIAN)
md.detail = False

_arr = None
def word(pc):
    off = elf.addr_to_off(pc)
    return struct.unpack_from('<I', raw, off)[0]

def prologue_score(pc):
    """0-3: how much pc looks like a function start."""
    score = 0
    try:
        w0 = word(pc)
    except Exception:
        return 0
    dis = list(md.disasm(raw[elf.addr_to_off(pc):elf.addr_to_off(pc)+16], pc))
    if not dis:
        return 0
    m0 = dis[0].mnemonic
    if m0 in ('stp',):
        ops = dis[0].op_str
        if ops.startswith('x29, x30') or 'x30' in ops.split(', ')[-1] if ops else False:
            score += 3
        else:
            score += 2
    elif m0 == 'sub' and dis[0].op_str.startswith('sp, sp'):
        score += 2
    elif m0 in ('pacibsp', 'bti'):
        score += 3
    elif m0 == 'str' and 'x30' in dis[0].op_str:
        score += 2
    elif m0 == 'ldr':
        score += 1
    # preceded by ret / b / br / padding?
    if pc >= TEXT_LO + 4:
        try:
            prev = list(md.disasm(raw[elf.addr_to_off(pc-4):elf.addr_to_off(pc)], pc-4))
            if prev and prev[0].mnemonic in ('ret', 'b', 'br', 'nop'):
                score += 1
        except Exception:
            pass
    return score

def find_func_start(anchor, max_back=0x8000):
    """Heuristic real function start for the code at `anchor`."""
    cands = []
    pc = anchor - 4
    lo = max(TEXT_LO, anchor - max_back)
    known = set(FUNCS)
    steps = 0
    while pc >= lo and steps < max_back // 4:
        w = word(pc)
        steps += 1
        # ret
        if w == 0xd65f03c0:
            nxt = pc + 4
            if nxt <= anchor:
                cands.append((nxt, 'post-ret'))
        if pc in known:
            cands.append((pc, 'known'))
        pc -= 4
    if not cands:
        return None
    # score candidates; prefer the LATEST strong candidate (closest to anchor)
    best = None
    for c, why in cands:
        s = prologue_score(c)
        if c in known:
            s += 1
        if s >= 3:
            if best is None or c > best[0]:
                best = (c, why, s)
    if best:
        return best[0]
    # fallback: latest known start
    i = bisect.bisect_right(FUNCS, anchor) - 1
    return FUNCS[i] if i >= 0 else None

def fs(pc):
    i = bisect.bisect_right(FUNCS, pc) - 1
    return FUNCS[i] if i >= 0 else None

def dis(f, n=30):
    off = elf.addr_to_off(f)
    out = []
    for ins in md.disasm(raw[off:off + n * 4], f):
        out.append(f"{ins.address:#x}: {ins.mnemonic:9s} {ins.op_str}")
        if len(out) >= n:
            break
    return out

def xref_str(s, wide=True, ascii_too=False):
    out = []
    if wide:
        a = elf.find_utf16(s)
        if a is not None:
            out += XREFS.get(a, [])
    if ascii_too:
        a = elf.find_ascii(s)
        if a is not None:
            out += XREFS.get(a, [])
    return sorted(set(out))

def strings_near(anchor, back=0x200, fwd=0x40):
    """UTF-16 strings referenced in [anchor-back, anchor+fwd]."""
    out = []
    pc = anchor - back
    while pc <= anchor + fwd:
        if pc >= TEXT_LO:
            w = word(pc)
            if (w & 0x9F000000) == 0x90000000:
                # adrp — resolve target
                i = (pc - TEXT_LO) // 4
                page = None
                r = core.adrp_decode(pc, w)
                if r:
                    page = r[1]
                    if i + 1 < len(st_arr):
                        w2 = st_arr[i + 1]
                        ad = core.add_imm_decode(w2)
                        if ad and ad[1] == r[0]:
                            t = page + ad[2]
                        else:
                            ld = core.ldrx_imm_decode(w2)
                            if ld and ld[1] == r[0]:
                                t = page + ld[2]
                            else:
                                t = None
                        if t is not None:
                            s = read_str(t)
                            if s and len(s) >= 3:
                                out.append((hex(pc), s[:80]))
        pc += 4
    return out

import array as _array
st_arr = _array.array('I')
st_arr.frombytes(raw[elf.addr_to_off(TEXT_LO):elf.addr_to_off(TEXT_LO) + TEXT_SZ])

def read_str(addr, wide=True):
    r = elf.read(addr, 600)
    if r is None:
        return None
    if wide:
        chars = []
        for i in range(0, len(r) - 1, 2):
            c = r[i] | (r[i + 1] << 8)
            if c == 0:
                return "".join(chars) if chars else None
            chars.append(chr(c))
    i = r.find(b'\0')
    return r[:i].decode('utf-8', 'replace') if i > 0 else None

def vtables_of(fn):
    return FVS.get(fn, [])

def callers_of(fn):
    return CALLERS.get(fn, [])

def callees_of(fn):
    return list(dict.fromkeys(CALLEES.get(fn, [])))
