# find82 — Erbium's 82 functions in libUnreal.so (ARM64)

Static-analysis toolchain that locates the 82 functions from
`Erbium/Erbium/Public/Finders.h` inside the Android `libUnreal.so`
(Fortnite 21.30.0-CL-21088273, UE 5.1) — replacing the 276 x64
signature scanners of the Windows build.

## Status (21.30)

| status | count | meaning |
|---|---|---|
| resolved | 44 | exact address baked (`kFn_*` in `android/Generated/offsets-21.30.h`) |
| ambiguous | 2 | candidate address; anchor shared by 2 functions (GiveAbility/GiveAbilityAndActivateOnce, StreamInMyBuilding/SelectAndSetupMyBuildingLevel) — disambiguate by disasm before hooking |
| todo_runtime | 45 | resolve at RUNTIME via UObject reflection (exactly how Windows Erbium does `DefaultObjImpl("NetDriver")->Vft[idx]`) — needs GObjects/GNames first |

Provenance for every entry: `android/Generated/functions-21.30.json`.

## How it works

```
build_state2.py      APK→libUnreal.so: APS2 relocations → got/relro maps,
                     .eh_frame function starts, BL call graph, vtable runs
build_branch_index.py  precomputed branch-target → sources index (all of .text)
h.py                 exploration helpers (disasm, xrefs, prologue scoring,
                     find_func_start with latest-strong-candidate picking)
bake82.py            string → xref → cold-fragment owner resolution
bake82_full.py       runs the full string-anchor table → results82.json
reflected.py         exact UFunction name → relro registration pair →
                     counter-thunk → tail-jump target (the native)
scan_fast.py         vtable-slot virtual-call site scanner
gen_functions_bake.py  assembles functions-*.json + the kFn_ header section
```

Key ARM64 findings encoded here:

- **Cold/hot function splitting**: UE_LOG bodies live in cold fragments
  (`.text.unlikely`-style regions) tail-jumped from the hot function body.
  A string xref is usually NOT in the function that owns it — resolve via
  the branch-into-fragment index, then `find_func_start` (eh_frame starts +
  prologue scoring + ret-boundaries).
- **Vtables**: contiguous runs of APS2 RELATIVE relocations in `.data.rel.ro`
  (0x2470808 bytes ≈ 3.19M entries → ~97k vtables). NULL pure-virtual slots
  and header pairs (offset-to-top=0, no-RTTI typeinfo=0) carry **no
  relocation** — the runs break there, so slot *indices* are unreliable
  across tables; use **slot offsets relative to a verified anchor** instead.
- **Verified NetDriver-family slots** (vtable containing them:
  UIpNetDriver + subclass, relro base region 0xb9d0xxx):
  `+0x2d8 InitBase, +0x2e0 InitConnect, +0x2e8 InitListen`.
  Windows x64 indices (0x79/0x7a/0x7b/0x7c) do NOT transfer — the ARM64
  UObject virtual block is ~29 slots shorter.
- **Native registration tables**: `(func_ptr, name_ptr)` pairs in
  `.data.rel.ro`; the registered function is a 5-instruction counter
  thunk ending in `b <native>` — follow the tail-jump for the real native.

## Reproduce

```bash
# extract libUnreal.so from the APK (analyze_libue4.py does this too)
unzip fortnite.apk lib/arm64-v8a/libUnreal.so
export LIBUNREAL=$PWD/lib/arm64-v8a/libUnreal.so

cd scripts/find82
python3 build_state2.py          # ~70s — writes lib_state2.pkl
python3 build_branch_index.py    # ~25s — writes branch_index.pkl
python3 bake82_full.py           # string anchors → results82.json
python3 reflected.py             # reflected-native table
python3 gen_functions_bake.py    # → android/Generated/functions-21.30.json
                                 #   + kFn_ section in offsets-21.30.h
```

Requires: `capstone` (`uv add capstone` / `pip install capstone`).
