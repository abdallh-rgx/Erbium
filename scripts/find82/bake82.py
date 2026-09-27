#!/usr/bin/env python3
"""bake82 — resolve Erbium's 82 functions in libUnreal.so 21.30 (ARM64).

Techniques:
  A. string_anchor: UTF-16 string → xref (cold fragment) → branch into it →
     owner function (hot) → function start
  B. first_bl: like Windows finders that take the first call from the anchor
     function (GetNetMode)
  C. vtable slots (verified pairs)
"""
import sys, re
sys.path.insert(0, '/home/z/Erbium/scripts/find82')
import h

TEXT_LO, TEXT_HI = h.TEXT_LO, h.TEXT_HI
ARR = h.st_arr
N = len(ARR)

import pickle, os
_BIDX = None
def _bidx():
    global _BIDX
    if _BIDX is None:
        _BIDX = pickle.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'branch_index.pkl'), 'rb'))
    return _BIDX

def branches_into(lo, hi):
    idx = _bidx()
    out = []
    # iterate candidate targets in range (index is dense — walk hi down to lo by 4)
    t = lo
    while t <= hi:
        srcs = idx.get(t)
        if srcs:
            out.extend(srcs)
        t += 4
    return out

def owner_of_string(s, wide=True, ascii_too=False, window=0x60):
    """string → (owners set[(f, pc)], xref pcs, owner pcs)"""
    xr = h.xref_str(s, wide=wide, ascii_too=ascii_too)
    if not xr:
        return set(), [], []
    owners = set()
    for x in xr:
        # cold fragment spans [x-0x20, x+window]; branches into it come from
        # the hot part
        for pc in branches_into(x - 0x10, x + min(window, 8)):
            f = h.find_func_start(pc)
            if f is not None:
                owners.add((f, pc))
    return owners, xr, [p for _, p in owners]

def first_bl(f, skip=0):
    """first BL callee of function f (optionally skipping n calls)"""
    cals = h.callees_of(f)
    # callees_of is keyed by exact function start; for fragment issues scan
    # the function body directly
    end = min((f + 0x800), TEXT_HI)
    got = []
    i = (f - TEXT_LO) // 4
    while TEXT_LO + i*4 < end:
        w = ARR[i]
        if (w & 0xFC000000) == 0x94000000:
            imm = w & 0x3FFFFFF
            if imm & 0x2000000: imm -= 0x4000000
            tgt = TEXT_LO + i*4 + imm*4
            got.append(tgt)
            if len(got) > skip + 3:
                break
        i += 1
    return got[skip] if len(got) > skip else (got[0] if got else None)
