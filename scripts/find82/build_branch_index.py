import sys, pickle
sys.path.insert(0, '/home/z/Erbium/scripts/find82')
import h
TEXT_LO, TEXT_HI = h.TEXT_LO, h.TEXT_HI
ARR = h.st_arr; N = len(ARR)
index = {}   # target_pc -> [source_pcs]
for i in range(N):
    w = ARR[i]
    tgt = None
    if (w & 0xFC000000) == 0x14000000:
        imm = w & 0x3FFFFFF
        if imm & 0x2000000: imm -= 0x4000000
        tgt = TEXT_LO + i*4 + imm*4
    elif (w & 0xFF000010) == 0x54000000:
        imm19 = (w >> 5) & 0x7FFFF
        if imm19 & 0x40000: imm19 -= 0x80000
        tgt = TEXT_LO + i*4 + imm19*4
    elif (w & 0x7F000000) in (0x34000000, 0x35000000):
        imm19 = (w >> 5) & 0x7FFFF
        if imm19 & 0x40000: imm19 -= 0x80000
        tgt = TEXT_LO + i*4 + imm19*4
    elif (w & 0x7E000000) == 0x36000000:
        imm14 = (w >> 5) & 0x3FFF
        if imm14 & 0x2000: imm14 -= 0x4000
        tgt = TEXT_LO + i*4 + imm14*4
    if tgt is not None and TEXT_LO <= tgt < TEXT_HI:
        index.setdefault(tgt, []).append(TEXT_LO + i*4)
pickle.dump(index, open('branch_index.pkl', 'wb'), protocol=4)
print("branch targets:", len(index))
