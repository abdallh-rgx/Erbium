# Erbium — Android ARM64 Port (In Progress)

Fork of [plooshi/Erbium](https://github.com/plooshi/Erbium) being ported to run **inside the Fortnite Android client (ARM64)**, so a single phone/tablet/emulator can **host and play** (LAN or solo).

> The Windows x64 codebase stays untouched on `main`. All Android work lives on the `android` branch.

---

## Architecture (Target)

```
┌─────────────────────────── Android device (ARM64) ───────────────────────────┐
│  Fortnite APK (patched via apktool)                                          │
│  ├── smali: onCreate() → System.loadLibrary("erbium")                        │
│  ├── lib/arm64-v8a/liberbium.so   ← THIS PORT (server, from Erbium C++ src)  │
│  │     • constructor → waits for libUE4.so/libUnreal.so                      │
│  │     • SDK::Init (GObjects/GNames via ARM64 anchors)                       │
│  │     • flips GIsClient/GIsServer, opens Athena_Terrain                     │
│  │     • hooks TickFlush etc. via embedded ARM64 Dobby (same approach as     │
│  │       Owen.c from Voltronite)                                             │
│  │     • listens 0.0.0.0:7777 (UDP) — LAN players join via Wi-Fi             │
│  └── lib/arm64-v8a/libowen.so     ← redirect hook (Owen.c, upstream)          │
│        • redirects Epic HTTP/EOS to backend at 127.0.0.1:3551                 │
└──────────────────────────────────────────────────────────────────────────────┘
         │ adb reverse (emulator) or same Wi-Fi
   Voltronite backend (Bun, :3551) + matchmaker (:8080) — fake Epic services
```

### Component map

| Concern | Windows x64 (upstream) | Android ARM64 (this port) |
|---|---|---|
| Entry | DLL injection → `DllMain` | `System.loadLibrary` → `__attribute__((constructor))` |
| Hooking | MinHook | Embedded ARM64 Dobby (as in Owen.c) |
| Module/scan | Memcury (WinAPI) | `/proc/self/maps` + `dl_iterate_phdr` |
| Patterns | x64 byte patterns (Finders.cpp) | offline analyzer → baked offsets (`android/Generated/*`) |
| Console/GUI | console + ImGui | logcat only (`tag: Erbium`) |
| Patches | `0xC3`/`mov eax,1` x64 | `RET`/`MOV W0,#1` ARM64 |
| Crash reporter | custom SEH | disabled v1 (tombstones via logcat) |

### Version support (v1 target)

**`18.40.0-CL-18167774`** (Chapter 2 Season 8, Nov 2021):
- Fully supported by upstream Erbium (3.4 → 19.40).
- Owen.c (Voltronite fork) already ships UE-HTTP hook offsets for `CL-18167774`.
- Outside the PartyHub problem range (12.61–15.50).
- Uptodown file id: `4114108` (142.8 MB APK).

Full matrix later: latest of every season (see `android/uptodown_versions.json` for all 370 Uptodown version ids).

---

## Repo layout (android branch)

```
android/                    build + platform layer
  CMakeLists.txt            NDK build (arm64-v8a) → liberbium.so
  src/entry.cpp             constructor, libUE4 wait loop, logcat bootstrap
  src/alog.h                __android_log wrappers
  src/win32_compat.h        Sleep/CreateThread/VirtualProtect/... shims
  src/platform.cpp          maps parsing, module base, mprotect helpers
  uptodown_versions.json    370 versions scraped from Uptodown API
scripts/
  analyze_libue4.py         offline ELF analyzer → baked offsets header
  get_apk.py                APK fetcher (direct URL / archive.org)
  patch_apk.sh              apktool decode → onCreate smali patch → build → sign
.github/workflows/
  smoke.yml                 x86_64: NDK cross-compile check (fast)
  bringup.yml               arm64: emulator + APK + crash detection (WIP)
docs/ANDROID-PORT.md        this file
```

## CI strategy

Two runner types, because tools and targets differ:

1. **`ubuntu-latest` (x86_64)** — compile `liberbium.so` with the standard NDK (NDK has no linux-arm64 host build). Fast, native.
2. **`ubuntu-24.04-arm` (ARM64)** — run the Android 10 (API 29) ARM64 emulator (QEMU, no KVM, SwiftShader GPU), install the patched APK, stream logcat, detect crashes/tombstones. Tailscale (`secrets.TAILSCALE_AUTHKEY`) lets you SSH into the runner live.

## Status

- [x] Fork with full upstream history (`main` untouched)
- [x] Actions enabled, `TAILSCALE_AUTHKEY` secret set
- [x] Android platform skeleton + NDK build
- [x] Smoke workflow (cross-compile)
- [ ] APK pipeline (analyze → patch → sign) validated on 18.40
- [ ] Erbium core compiled for Android (Finders/Hooking port)
- [ ] Emulator bring-up (app boots, liberbium loads, engine init)
- [ ] Backend integration (Voltronite + Owen.c, adb reverse)
- [ ] Solo match entry on emulator
- [ ] Multi-version matrix
