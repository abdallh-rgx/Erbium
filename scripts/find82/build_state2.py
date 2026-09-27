#!/usr/bin/env python3
"""Rebuild lib state with EXACT function boundaries from .eh_frame_hdr."""
import pickle, struct, array, time
import core

t0 = time.time()
import os
LIBP = os.environ.get('LIBUNREAL') or sys.argv[1] if len(sys.argv) > 1 else os.environ.get('LIBUNREAL') or 'libUnreal.so'
raw = open(LIBP, 'rb').read()
elf = core.Elf64(raw)
lib = core.Lib.__new__(core.Lib)
lib.path = LIBP
lib.b = raw
lib.elf = elf
text = elf.section(".text")
lib.TEXT_LO, lib.TEXT_SZ = text["addr"], text["size"]
lib.TEXT_HI = lib.TEXT_LO + lib.TEXT_SZ
lib.text_arr = array.array("I")
lib.text_arr.frombytes(raw[text["offset"]:text["offset"] + text["size"]])
lib.got_map, lib.relro_map = core.decode_aps2(elf)
print(f"[{time.time()-t0:.0f}s] aps2 decoded: relro={len(lib.relro_map)} got={len(lib.got_map)}")

# exact function starts
starts = pickle.load(open('eh_frame_starts.pkl','rb'))
TEXT_LO, TEXT_HI = lib.TEXT_LO, lib.TEXT_HI
extra = set()
for name, value, size in elf.dynsyms():
    if value and TEXT_LO <= value < TEXT_HI:
        extra.add(value)
for tgt in list(lib.relro_map.values()) + list(lib.got_map.values()):
    if tgt and TEXT_LO <= tgt < TEXT_HI:
        extra.add(tgt)
# FDE starts that fall inside .text
lib.funcs = sorted(set(s for s in starts if TEXT_LO <= s < TEXT_HI) | extra)
lib.func_set = set(lib.funcs)
print(f"[{time.time()-t0:.0f}s] functions: {len(lib.funcs)} (eh_frame + {len(extra)} extras)")

# BL edges with proper owners
import bisect
lib.func_index = {f: i for i, f in enumerate(lib.funcs)}
def fs(pc):
    i = bisect.bisect_right(lib.funcs, pc) - 1
    return lib.funcs[i] if i >= 0 else None
lib.func_start_of = fs
lib.callers = {}
lib.callees = {}
callers = {}
callees = {}
n = len(lib.text_arr)
for i in range(n):
    w = lib.text_arr[i]
    if (w & 0xFC000000) == 0x94000000:
        pc = TEXT_LO + i * 4
        imm26 = w & 0x3FFFFFF
        if imm26 & 0x2000000: imm26 -= 0x4000000
        tgt = pc + imm26 * 4
        f = fs(pc)
        if f is not None and TEXT_LO <= tgt < TEXT_HI:
            callers.setdefault(tgt, set()).add(f)
            callees.setdefault(f, []).append(tgt)
lib.callers = callers
lib.callees = callees
print(f"[{time.time()-t0:.0f}s] call graph built")

# vtables: contiguous runs of pointers into .text (or 0), gaps = header pairs
# vtable base = first entry after a (v, 0) header pair where v==0|small and
# the run is preceded by no-reloc padding. Detect runs of reloc entries
# separated by ≤2 non-reloc slots (headers/NULL pure-virtuals).
vtables = []           # (base, {slot: func})
addrs = sorted(lib.relro_map)
runs = []
cur_start = None
prev_addr = None
for a in addrs:
    tgt = lib.relro_map[a]
    ok = tgt == 0 or (TEXT_LO <= tgt < TEXT_HI)
    if not ok:
        continue
    if prev_addr is None or a - prev_addr > 24:  # > 3 slots gap → new vtable
        if prev_addr is not None and runs:
            pass
        cur_start = a
    runs.append((a, tgt))
    prev_addr = a
# group runs: split where gap > 8 but allow gap of 16/24 (null slots/padding)
groups = []
cur = []
prev_a = None
for a, tgt in runs:
    if prev_a is not None and a - prev_a > 8:
        if len(cur) >= 2:
            groups.append(cur)
        cur = []
    cur.append((a, tgt))
    prev_a = a
if len(cur) >= 2:
    groups.append(cur)
# convert to vtable dicts with slot indexes relative to first entry
vtbl = []
for g in groups:
    base = g[0][0]
    slots = {}
    for a, tgt in g:
        slots[(a - base) // 8] = tgt
    if len(slots) >= 2:
        vtbl.append((base, slots))
lib.vtables = vtbl
fv = {}
for vbase, slots in vtbl:
    for slot, fn in slots.items():
        if fn:
            fv.setdefault(fn, []).append((vbase, slot))
lib.func_vtables = fv
print(f"[{time.time()-t0:.0f}s] vtables: {len(vtbl)}  funcs-in-vtables: {len(fv)}")

# xrefs (same algorithm as core)
from collections import defaultdict
xrefs = defaultdict(list)
pending = {}
for i in range(n):
    pc = TEXT_LO + i * 4
    w = lib.text_arr[i]
    r = core.adrp_decode(pc, w)
    if r:
        pending[pc] = (r[0], r[1])
        if len(pending) > 64:
            del pending[min(pending)]
        continue
    ad = core.add_imm_decode(w)
    if ad:
        for ppc in list(pending):
            rd, page = pending[ppc]
            if rd == ad[1] and pc - ppc <= 0x2000:
                xrefs[page + ad[2]].append(ppc)
                del pending[ppc]
        continue
    ld = core.ldrx_imm_decode(w)
    if ld:
        for ppc in list(pending):
            rd, page = pending[ppc]
            if rd == ld[1] and pc - ppc <= 0x2000:
                xrefs[page + ld[2]].append(ppc)
                del pending[ppc]
        continue
    ll = core.ldr_lit_decode(pc, w)
    if ll:
        xrefs[ll[1]].append(pc)
lib.xrefs = dict(xrefs)
print(f"[{time.time()-t0:.0f}s] xrefs: {len(lib.xrefs)}")

state = {
 'funcs': lib.funcs, 'vtables': lib.vtables, 'xrefs': lib.xrefs,
 'callees': {k: v for k, v in lib.callees.items()},
 'callers': {k: list(v) for k, v in lib.callers.items()},
 'got_map': lib.got_map, 'relro_map': lib.relro_map,
 'func_vtables': lib.func_vtables,
}
pickle.dump(state, open('lib_state2.pkl', 'wb'), protocol=4)
print(f"[{time.time()-t0:.0f}s] saved lib_state2.pkl")
