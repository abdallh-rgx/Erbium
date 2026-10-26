#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  patch_apk_fast.sh — surgical APK patch for Erbium injection (zip surgery)
#
#  Instead of a full apktool round-trip (decode 285MB of libs + rebuild),
#  we do a direct zip-level patch:
#    1. baksmali ONLY classes.dex
#    2. edit GameActivity.smali:
#         a. onCreate: loadLibrary("erbium") right after loadLibrary("sinum")
#            (DHyper's OGFN builds load their redirect lib there; vanilla
#             builds get it injected at the top of onCreate)
#         b. append dispatchTouchEvent: 4-finger touch → console
#         c. append forceShowConsoleWindow_test → AndroidThunkJava_ShowConsoleWindow
#    3. reassemble classes.dex (smali lib inside apktool.jar via a tiny
#       wrapper — no separate smali download needed)
#    4. python zip surgery: replace classes.dex + add lib/arm64-v8a/liberbium.so
#    5. sign (uber-apk-signer)
#
#  Requirements: java 17+ (JDK with javac for the wrapper), python3, unzip, zip
#
#  Usage: patch_apk_fast.sh <in.apk> <out.apk> <liberbium.so> [workdir]
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail

IN_APK="$1"; OUT_APK="$2"; LIBERBIUM="$3"; WORK="${4:-patch-work}"

command -v javac >/dev/null || { echo "need a JDK (javac)"; exit 1; }
command -v python3 >/dev/null || { echo "need python3"; exit 1; }

IN_APK="$(realpath -m "$IN_APK")"
OUT_APK="$(realpath -m "$OUT_APK")"
LIBERBIUM="$(realpath -m "$LIBERBIUM")"
WORK="$(realpath -m "$WORK")"

BAKSMALI_JAR="${BAKSMALI_JAR:-$WORK/baksmali-2.5.2.jar}"
APKTOOL_JAR="${APKTOOL_JAR:-$WORK/apktool_2.9.3.jar}"
UBERSIGNER_JAR="${UBERSIGNER_JAR:-$WORK/uber-apk-signer-1.3.0.jar}"

mkdir -p "$WORK"
cd "$WORK"

# ── fetch tools if missing ─────────────────────────────────────────────────
if [ ! -f "$BAKSMALI_JAR" ]; then
  curl -fsSL -o "$BAKSMALI_JAR" "https://bitbucket.org/JesusFreke/smali/downloads/baksmali-2.5.2.jar" \
    || curl -fsSL -o "$BAKSMALI_JAR" "https://github.com/google/smali/releases/download/v2.5.2/baksmali-2.5.2.jar"
fi
if [ ! -f "$APKTOOL_JAR" ]; then
  curl -fsSL -o "$APKTOOL_JAR" "https://bitbucket.org/iBotPeaches/apktool/downloads/apktool_2.9.3.jar"
fi
if [ ! -f "$UBERSIGNER_JAR" ]; then
  curl -fsSL -o "$UBERSIGNER_JAR" "https://github.com/patrickfav/uber-apk-signer/releases/download/v1.3.0/uber-apk-signer-1.3.0.jar"
fi

# ── 1. baksmali classes.dex ────────────────────────────────────────────────
rm -rf smali-in smali-out
mkdir -p smali-in
unzip -o -q "$IN_APK" "classes.dex" -d smali-in
[ -f smali-in/classes.dex ] || { echo "no classes.dex in APK"; exit 1; }
java -jar "$BAKSMALI_JAR" d smali-in/classes.dex -o smali-out

GA="smali-out/com/epicgames/ue4/GameActivity.smali"
[ -f "$GA" ] || { echo "GameActivity.smali not found after baksmali"; exit 1; }
echo "[patch] GameActivity.smali: $(wc -l < "$GA") lines"

# ── 2. patch the smali ─────────────────────────────────────────────────────
python3 - "$GA" <<'PY'
import sys
p = sys.argv[1]
src = open(p).read()

if 'const-string v0, "erbium"' not in src:
    anchor = '''    const-string v0, "sinum"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
    inject = anchor + '''
    const-string v0, "erbium"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
    if anchor in src:
        src = src.replace(anchor, inject, 1)
        print("[patch] injected loadLibrary(erbium) after sinum")
    else:
        import re
        m = re.search(r'(\.method public onCreate\(Landroid/os/Bundle;\)V\n\s*\.registers \d+\n)', src)
        if not m:
            print("[patch] ERROR: no sinum anchor and no onCreate"); sys.exit(1)
        inject = m.group(1) + '''    const-string v0, "erbium"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
'''
        src = src[:m.start()] + inject + src[m.end():]
        print("[patch] injected loadLibrary(erbium) at top of onCreate (vanilla build)")

if 'forceShowConsoleWindow_test' not in src:
    src += '''

.method public dispatchTouchEvent(Landroid/view/MotionEvent;)Z
    .registers 4

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
'''
    print("[patch] appended dispatchTouchEvent + forceShowConsoleWindow_test")

if 'AndroidThunkJava_ShowConsoleWindow(Ljava/lang/String;)V' not in src:
    print("[patch] WARNING: AndroidThunkJava_ShowConsoleWindow missing upstream!")

open(p, 'w').write(src)
PY

# ── 3. reassemble classes.dex (smali lib inside apktool.jar) ───────────────
cat > Assemble.java <<'EOF'
import brut.androlib.mod.SmaliMod;
import com.android.tools.smali.dexlib2.Opcodes;
import com.android.tools.smali.dexlib2.writer.builder.DexBuilder;
import com.android.tools.smali.dexlib2.writer.io.FileDataStore;
import java.io.File;
import java.nio.file.*;
import java.util.stream.*;

public class Assemble {
    public static void main(String[] args) throws Exception {
        File dir = new File(args[0]);
        File out = new File(args[1]);
        int api = args.length > 2 ? Integer.parseInt(args[2]) : 29;
        DexBuilder dex = new DexBuilder(Opcodes.forApi(api));
        try (Stream<Path> paths = Files.walk(dir.toPath())) {
            for (Path p : paths.filter(x -> x.toString().endsWith(".smali")).collect(Collectors.toList())) {
                SmaliMod.assembleSmaliFile(p.toFile(), dex, api, false, false);
            }
        }
        dex.writeTo(new FileDataStore(out));
        System.out.println("assembled " + args[0] + " -> " + args[1] + " (" + out.length() + " bytes)");
    }
}
EOF
javac -cp "$APKTOOL_JAR" Assemble.java -d .
java -cp "$APKTOOL_JAR:." Assemble smali-out classes-new.dex 29
ls -la classes-new.dex

# ── 4. zip surgery (python for correct entry names + compression) ──────────
python3 - "$IN_APK" classes-new.dex "$LIBERBIUM" patched-unsigned.apk <<'PY'
import sys, zipfile
in_apk, dex_path, lib_path, out_apk = sys.argv[1:5]

zin = zipfile.ZipFile(in_apk, 'r')
zout = zipfile.ZipFile(out_apk, 'w', zipfile.ZIP_DEFLATED, compresslevel=1)

new_dex = open(dex_path, 'rb').read()
new_lib = open(lib_path, 'rb').read()
replaced = added = False

for item in zin.infolist():
    name = item.filename
    if name == 'classes.dex':
        zi = zipfile.ZipInfo(name, date_time=item.date_time)
        zi.compress_type = zipfile.ZIP_DEFLATED
        zout.writestr(zi, new_dex)
        replaced = True
        continue
    data = zin.read(name)
    zi = zipfile.ZipInfo(name, date_time=item.date_time)
    zi.compress_type = item.compress_type
    zi.external_attr = item.external_attr
    zout.writestr(zi, data)

zi = zipfile.ZipInfo('lib/arm64-v8a/liberbium.so')
zi.compress_type = zipfile.ZIP_DEFLATED
zout.writestr(zi, new_lib)

zout.close(); zin.close()
z = zipfile.ZipFile(out_apk)
assert replaced, "classes.dex not replaced!"
assert 'lib/arm64-v8a/liberbium.so' in z.namelist(), "liberbium.so missing!"
print(f"[zip] entries={len(z.namelist())} classes.dex={z.getinfo('classes.dex').file_size}B liberbium.so=OK")
PY

# ── 5. sign ────────────────────────────────────────────────────────────────
java -jar "$UBERSIGNER_JAR" -a patched-unsigned.apk --overwrite --allowResign >/dev/null
cp patched-unsigned.apk "$OUT_APK"
echo "[patch] DONE → $OUT_APK ($(du -h "$OUT_APK" | cut -f1))"
