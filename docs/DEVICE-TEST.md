# Erbium Android — Device Test Guide / دليل الاختبار على الجهاز

**Status / الحالة (2026-09-26):** v1 pipeline is COMPLETE and CI-proven up to engine init.
The emulator (ndk_translation) crawls on the 228 MB engine — a **real ARM64 device**
is the fast path to the v1 milestone.

**What v1 does / ما يفعله الإصدار الأول:**
1. `liberbium.so` loads from the patched `SplashActivity.onCreate`
2. Waits for `libUnreal.so`, verifies the bake (`++Fortnite+Release-21.30-CL-21088273`)
3. Installs the Owen.c redirect (Epic traffic → `http://127.0.0.1:3551`)
4. Waits for `GEngine`, then flips **GIsClient=false / GIsServer=true**
5. Dispatches `open Artemis_Terrain` through the game's own `nativeConsoleCommand`

---

## Get the patched APK / الحصول على النسخة المعدّلة

From the latest successful `android-bringup` run → **Artifacts** → download, **or**
decrypt the release asset:

```bash
gh release download apk-21.30.0-21088273 --repo abdallh-rgx/Erbium \
  --pattern "erbium-patched-*.apk.enc" --output patched.apk.enc
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$APK_CRYPT_KEY" \
  -in patched.apk.enc -out fortnite-erbium-patched.apk
```

(`APK_CRYPT_KEY` is the repo secret; for local use ask the CI to print or run the
bring-up with `apk_url` pointing at your own fresh Uptodown link.)

## Install on your phone / التثبيت على هاتفك

```bash
adb uninstall com.epicgames.fortnite   # remove the store/original build first
adb install fortnite-erbium-patched.apk
```

## Run the backend on the device (or PC) / تشغيل السيرفر الخلفي

On-device (Termux) — same as your owenlauncher setup:

```bash
git clone https://github.com/abdallh-rgx/Voltronite && cd Voltronite
bun install && bun src/app.ts     # listens on 0.0.0.0:3551
```

If you run Voltronite on your PC instead, tell Erbium by rebuilding with
`ERBIUM_BACKEND_URL=http://<PC-IP>:3551` (or use adb reverse:
`adb reverse tcp:3551 tcp:3551` — then 127.0.0.1 on the phone still works).

## Watch the milestones / تابع المعالم في logcat

```bash
adb logcat -c && adb logcat -s Erbium Erbium-Owen UE
```

You should see, in order:

```
Erbium  : === Erbium Android bootstrap ===
Erbium  : engine module: libUnreal.so base=0x... 
Erbium-Owen: UE hooks installed: ProcessRequest=0x89f02f0 ...
Erbium  : release probe OK: ++Fortnite+Release-21.30-CL-21088273
Erbium  : waiting for GEngine (slot 0xde34970)...
Erbium  : GEngine = 0x...
Erbium  : POST-FLIP: GIsClient=0 GIsServer=1   ← the process is now a game server
Erbium  : dispatching console command: open Artemis_Terrain
```

Then the map loads (frontend travels to Artemis). If the flip+open happen but the
map stalls, that's the phase-3b work (LocalPlayers removal + NetDriver listen setup).

## Report back / أرسل لنا

Paste the `adb logcat -s Erbium Erbium-Owen` output (plus any `UE`/`LogNet` lines
after the flip) from the CI issues page or the repo's Discussions.
