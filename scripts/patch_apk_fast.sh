#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  patch_apk_fast.sh — surgical APK patch for Erbium injection (zip surgery)
#
#  Instead of a full apktool round-trip (decode 285MB of libs + rebuild),
#  we do a direct zip-level patch:
#    1. baksmali ONLY classes.dex
#    2. edit GameActivity.smali:
#         a. onCreate: loadLibrary("erbium") right after loadLibrary("sinum")
#            (DHyper's OGFN builds load their redirect lib there; upstream
#             Fortnite builds have no sinum — we inject after the first
#             loadLibrary OR at the top of onCreate)
#         b. add dispatchTouchEvent: 4-finger touch → forceShowConsoleWindow()
#         c. add forceShowConsoleWindow_test → AndroidThunkJava_ShowConsoleWindow
#    3. smali reassemble → replace classes.dex in the zip
#    4. add lib/arm64-v8a/liberbium.so
#    5. zipalign + sign (uber-apk-signer debug key)
#
#  Usage: patch_apk_fast.sh <in.apk> <out.apk> <liberbium.so> [workdir]
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail

IN_APK="$1"; OUT_APK="$2"; LIBERBIUM="$3"; WORK="${4:-patch-work}"

command -v java >/dev/null || { echo "need java 17+"; exit 1; }

IN_APK="$(realpath -m "$IN_APK")"
OUT_APK="$(realpath -m "$OUT_APK")"
LIBERBIUM="$(realpath -m "$LIBERBIUM")"
WORK="$(realpath -m "$WORK")"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BAKSMALI_JAR="${BAKSMALI_JAR:-$WORK/baksmali-2.5.2.jar}"
SMALI_JAR="${SMALI_JAR:-$WORK/smali-2.5.2.jar}"
UBERSIGNER_JAR="${UBERSIGNER_JAR:-$WORK/uber-apk-signer-1.3.0.jar}"

mkdir -p "$WORK"
cd "$WORK"

# ── fetch tools if missing ─────────────────────────────────────────────────
if [ ! -f "$BAKSMALI_JAR" ]; then
  curl -fsSL -o "$BAKSMALI_JAR" "https://bitbucket.org/JesusFreke/smali/downloads/baksmali-2.5.2.jar"
fi
if [ ! -f "$SMALI_JAR" ]; then
  curl -fsSL -o "$SMALI_JAR" "https://bitbucket.org/JesusFreke/smali/downloads/smali-2.5.2.jar"
fi
if [ ! -f "$UBERSIGNER_JAR" ]; then
  curl -fsSL -o "$UBERSIGNER_JAR" "https://github.com/patrickfav/uber-apk-signer/releases/download/v1.3.0/uber-apk-signer-1.3.0.jar"
fi

# ── extract classes.dex ────────────────────────────────────────────────────
rm -rf smali-in smali-out
mkdir -p smali-in
unzip -o -q "$IN_APK" "classes.dex" "classes2.dex" "classes3.dex" "classes4.dex" -d smali-in || true
[ -f smali-in/classes.dex ] || { echo "no classes.dex in APK"; exit 1; }

java -jar "$BAKSMALI_JAR" d smali-in/classes.dex -o smali-out

GA="smali-out/com/epicgames/ue4/GameActivity.smali"
[ -f "$GA" ] || { echo "GameActivity.smali not found after baksmali"; exit 1; }
echo "[patch] GameActivity.smali: $(wc -l < "$GA") lines"

# ── 1. inject loadLibrary("erbium") into onCreate ─────────────────────────
if ! grep -q 'loadLibrary("erbium")\|const-string v0, "erbium"' "$GA"; then
  if grep -q 'const-string v0, "sinum"' "$GA"; then
    # DHyper OGFN build: right after the sinum load
    python3 - "$GA" <<'PY'
import sys, re
p = sys.argv[1]
src = open(p).read()
anchor = '''    const-string v0, "sinum"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
inject = anchor + '''
    const-string v0, "erbium"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
if anchor not in src:
    print("[patch] ERROR: sinum anchor not found"); sys.exit(1)
src = src.replace(anchor, inject, 1)
open(p, 'w').write(src)
print("[patch] injected loadLibrary(erbium) after sinum")
PY
  else
    # vanilla Fortnite build: at the very top of onCreate
    python3 - "$GA" <<'PY'
import sys, re
p = sys.argv[1]
src = open(p).read()
m = re.search(r'(\.method public onCreate\(Landroid/os/Bundle;\)V\n\s*\.registers \d+\n)', src)
if not m:
    print("[patch] ERROR: onCreate not found"); sys.exit(1)
inject = m.group(1) + '''    const-string v0, "erbium"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
src = src[:m.start()] + inject + src[m.end():]
open(p, 'w').write(src)
print("[patch] injected loadLibrary(erbium) at top of onCreate")
PY
  fi
fi

# ── 2. add dispatchTouchEvent + forceShowConsoleWindow_test ───────────────
if ! grep -q "forceShowConsoleWindow_test" "$GA"; then
  cat >> "$GA" <<'SMALI'

.method public dispatchTouchEvent(Landroid/view/MotionEvent;)Z
    .registers 4

    # p0 = GameActivity (this)
    # p1 = MotionEvent

    if-eqz p1, :cond_pass

    invoke-virtual {p1}, Landroid/view/MotionEvent;->getPointerCount()I
    move-result v0

    const/4 v1, 0x4
    if-ne v0, v1, :cond_pass

    invoke-virtual {p0}, Lcom/epicgames/ue4/GameActivity;->forceShowConsoleWindow_test()V

    :cond_pass
    invoke-super {p0, p1}, Landroid/app/NativeActivity;->dispatchTouchEvent(Landroid/view/MotionEvent;)Z
    move-result v0

    return v0
.end method

.method public forceShowConsoleWindow_test()V
    .registers 2

    const-string v0, "ASTC,ETC2"
    invoke-virtual {p0, v0}, Lcom/epicgames/ue4/GameActivity;->AndroidThunkJava_ShowConsoleWindow(Ljava/lang/String;)V

    return-void
.end method
SMALI
  echo "[patch] appended dispatchTouchEvent + forceShowConsoleWindow_test"
fi

# verify the console method exists (upstream UE4 GameActivity has it)
grep -q "AndroidThunkJava_ShowConsoleWindow(Ljava/lang/String;)V" "$GA" \
  || echo "[patch] WARNING: AndroidThunkJava_ShowConsoleWindow not found in GameActivity!"

# ── 3. reassemble classes.dex ─────────────────────────────────────────────
rm -f classes-new.dex
java -jar "$SMALI_JAR" a smali-out -o classes-new.dex
ls -la classes-new.dex

# ── 4. zip surgery ────────────────────────────────────────────────────────
rm -f patched-unsigned.apk
cp "$IN_APK" patched-unsigned.apk
# replace classes.dex (store the rest untouched)
zip -q patched-unsigned.apk classes-new.dex   # adds at root — wrong name!
# do it properly: update the entry named classes.dex
zip -q -d patched-unsigned.apk classes.dex
cd "$WORK" && mkdir -p ziproot && cp classes-new.dex ziproot/classes.dex && cd ziproot && zip -q ../patched-unsigned.apk classes.dex && cd "$WORK"
# add liberbium.so
mkdir -p lib/arm64-v8a
cp "$LIBERBIUM" lib/arm64-v8a/liberbium.so
cd lib && zip -q ../patched-unsigned.apk arm64-v8a/liberbium.so && cd "$WORK"
echo "[patch] zip updated:"
unzip -l patched-unsigned.apk | grep -E "classes.dex|liberbium" || true

# ── 5. sign ───────────────────────────────────────────────────────────────
java -jar "$UBERSIGNER_JAR" -a patched-unsigned.apk --overwrite --allowResign
cp patched-unsigned-aligned-debugSigned.apk "$OUT_APK" 2>/dev/null || cp patched-unsigned-debugSigned.apk "$OUT_APK" 2>/dev/null || {
  # newer uber-signer output name
  ls -la
  cp "$(ls patched-unsigned*Signed*.apk | head -1)" "$OUT_APK"
}
echo "[patch] DONE → $OUT_APK ($(du -h "$OUT_APK" | cut -f1))"
