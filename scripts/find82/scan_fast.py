#!/usr/bin/env python3
"""Fast virtual-call scan for specific slots (text array based)."""
import sys, time, json, array, struct
sys.path.insert(0, '/home/z/Erbium/scripts/find82')
import h

TEXT_LO = h.TEXT_LO
arr = h.st_arr  # preloaded array from h.py
N = len(arr)

def find_vcalls_fast(slot_off):
    sites = []
    vtbl_reg = {}
    slot_reg = {}
    for i in range(N):
        w = arr[i]
        if (w & 0xFFC00000) == 0xF9400000:
            imm = ((w >> 10) & 0xFFF) * 8
            if imm == 0:
                rn = (w >> 5) & 31
                if rn != 31:
                    vtbl_reg[w & 31] = i
            elif imm == slot_off:
                slot_reg[w & 31] = (i, (w >> 5) & 31)
        elif (w & 0xFFFFFC1F) == 0xD63F0000:
            r = (w >> 5) & 31
            if r in slot_reg:
                ppc, rn = slot_reg[r]
                if rn in vtbl_reg and ppc > vtbl_reg[rn] and ppc - vtbl_reg[rn] <= 4:
                    sites.append((TEXT_LO + vtbl_reg[rn]*4, TEXT_LO + ppc*4, TEXT_LO + i*4))
        if i & 0x3FF == 0x3FF:
            vtbl_reg.clear(); slot_reg.clear()
    return sites

slots = [int(x) for x in sys.argv[1:]]
out = {}
for slot in slots:
    t = time.time()
    out[slot] = find_vcalls_fast(slot * 8)
    print(f"slot {slot}: {len(out[slot])} sites ({time.time()-t:.0f}s)", flush=True)
json.dump({str(k): v for k, v in out.items()}, open('/home/z/Erbium/scripts/find82/vcalls_fast.json', 'w'))
print("saved")
