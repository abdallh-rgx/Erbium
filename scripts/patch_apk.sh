#!/usr/bin/env bash
# patch_apk.sh — decompile APK, inject loadLibrary("erbium") into the launcher
# activity's onCreate(), add our native libs, rebuild, sign.
#
# Usage: patch_apk.sh <in.apk> <out.apk> <workdir>
set -euo pipefail

IN_APK="$1"
OUT_APK="$2"
WORK="$3"
LIBERBIUM="${4:-build/liberbium.so}"

command -v java >/dev/null || { echo "need java 17+"; exit 1; }

APKTOOL_JAR="${APKTOOL_JAR:-$WORK/apktool.jar}"
UBERSIGNER_JAR="${UBERSIGNER_JAR:-$WORK/uber-apk-signer.jar}"
APKTOOL_VER="${APKTOOL_VER:-2.9.3}"

mkdir -p "$WORK"
cd "$WORK"

# ── fetch tools if missing ────────────────────────────────────────────────
if [ ! -f "$APKTOOL_JAR" ]; then
  curl -fsSL -o "$APKTOOL_JAR" "https://bitbucket.org/iBotPeaches/apktool/downloads/apktool_${APKTOOL_VER}.jar"
fi
if [ ! -f "$UBERSIGNER_JAR" ]; then
  curl -fsSL -o "$UBERSIGNER_JAR" "https://github.com/patrickfav/uber-apk-signer/releases/download/v1.3.0/uber-apk-signer-1.3.0.jar"
fi

# ── decode ────────────────────────────────────────────────────────────────
rm -rf decoded
java -jar "$APKTOOL_JAR" decode -f -o decoded "$IN_APK"

# ── find launcher activity from the manifest ──────────────────────────────
MANIFEST="decoded/AndroidManifest.xml"
LAUNCHER=$(python3 - <<'PY'
import xml.etree.ElementTree as ET
ns = {'a': 'http://schemas.android.com/apk/res/android'}
tree = ET.parse('decoded/AndroidManifest.xml')
root = tree.getroot()
# gather activities with LAUNCHER intent-filter
launchers = []
for act in root.iter('activity'):
    for flt in act.iter('intent-filter'):
        cats = [c.get('{http://schemas.android.com/apk/res/android}name') for c in flt.iter('category')]
        acts = [a.get('{http://schemas.android.com/apk/res/android}name') for a in flt.iter('action')]
        if any(c == 'android.intent.category.LAUNCHER' for c in cats) and \
           any(a == 'android.intent.action.MAIN' for a in acts):
            n = act.get('{http://schemas.android.com/apk/res/android}name')
            if n: launchers.append(n)
print(launchers[0] if launchers else 'NONE')
PY
)
echo "launcher activity: $LAUNCHER"
[ "$LAUNCHER" != "NONE" ] || { echo "no launcher activity found"; exit 1; }

# Convert activity name to smali path (handle .Foo → package + .Foo, and full names)
SMALI_PATH=$(python3 - "$LAUNCHER" <<'PY'
import sys, re
name = sys.argv[1].strip()
if name.startswith('.'):
    # need package from apktool.yml
    import yaml
    with open('decoded/apktool.yml') as f:
        y = yaml.safe_load(f)
    pkg = y.get('renameManifestPackage') or y.get('packageInfo', {}).get('renameManifestPackage') or ''
    # fallback: read from manifest root
    if not pkg:
        import xml.etree.ElementTree as ET
        root = ET.parse('decoded/AndroidManifest.xml').getroot()
        pkg = root.get('package')
    name = pkg + name
path = name.replace('.', '/')
print(f"decoded/smali/{path}.smali")
PY
)
echo "smali file: $SMALI_PATH"
[ -f "$SMALI_PATH" ] || { echo "smali file not found"; exit 1; }

# ── inject loadLibrary at the very top of onCreate ────────────────────────
python3 - "$SMALI_PATH" <<'PY'
import sys, re
path = sys.argv[1]
src = open(path, encoding='utf-8').read()
if 'loadLibrary("erbium")' in src or '"erbium"' in src:
    print("already patched")
    sys.exit(0)
m = re.search(r'(\.method\s+(?:public|protected)\s+onCreate\(Landroid/os/Bundle;\)V\s*\n(?:    \.locals[^\n]*\n)?)', src)
if not m:
    print("ERROR: onCreate not found")
    sys.exit(1)
inject = (
    "    const-string v0, \"erbium\"\n"
    "    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V\n"
)
idx = m.end()
# skip past any .annotation ... .end annotation prologue blocks — smali
# requires annotations to precede instructions, so we must inject after them
while True:
    ann = re.match(r'(    \.annotation[^\n]*\n(?:.*?\n)*?    \.end annotation\n)', src[idx:])
    if not ann:
        break
    idx += ann.end()
src = src[:idx] + inject + src[idx:]
open(path, 'w', encoding='utf-8').write(src)
print("injected loadLibrary into onCreate")
PY

# ── drop our native lib next to the game's ────────────────────────────────
if [ -f "$LIBERBIUM" ]; then
  mkdir -p decoded/lib/arm64-v8a
  cp "$LIBERBIUM" decoded/lib/arm64-v8a/liberbium.so
  echo "added liberbium.so to lib/arm64-v8a/"
else
  echo "WARNING: $LIBERBIUM not found — building APK without it (bootstrap test only)"
fi

# Old APKs + Android 12+ emulators: force extractNativeLibs so our .so loads
# from the data dir, and lower targetSdk if needed.
python3 - <<'PY'
import re
p = 'decoded/AndroidManifest.xml'
s = open(p, encoding='utf-8').read()
# ensure extractNativeLibs=true on <application>
def fix_app_attrs(m):
    tag = m.group(0)
    if 'extractNativeLibs' in tag:
        tag = re.sub(r'extractNativeLibs="[^"]*"', 'extractNativeLibs="true"', tag)
    else:
        tag = tag[:-1] + ' android:extractNativeLibs="true">'
    return tag
s = re.sub(r'<application[^>]*>', fix_app_attrs, s, count=1)
open(p, 'w', encoding='utf-8').write(s)
print("manifest: extractNativeLibs=true")
PY

# ── rebuild + sign ────────────────────────────────────────────────────────
java -jar "$APKTOOL_JAR" build decoded -o "$WORK/unsigned.apk" || {
  echo "apktool build failed — try with --use-aapt2"; exit 1; }

java -jar "$UBERSIGNER_JAR" -a "$WORK/unsigned.apk" -o "$WORK/signed" --allowResign
cp "$WORK"/signed/unsigned-aligned-debugSigned.apk "$OUT_APK" 2>/dev/null || \
  cp "$WORK"/signed/*debugSigned*.apk "$OUT_APK"

echo "PATCHED APK: $OUT_APK"
