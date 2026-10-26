#!/usr/bin/env python3
"""make_manifest.py — generate LawinServer Config/injected_manifests.json
from a Fortnite APK's embedded cloudcontent.json.

Replicates OGFN Manager's BackendManager.getInstalledBuilds() exactly (the
JSON it writes on-device when its backend runs) — but on the HOST, so the
host-node LawinServer (adb-reverse architecture) can serve it.

Usage: make_manifest.py <fortnite.apk> <lawinserver_dir>
Creates <lawinserver_dir>/Config/injected_manifests.json
"""
import json
import re
import sys
import zipfile


def version_key(build_version):
    """'++Fortnite+Release-18.40-CL-...' → JS Number('18.40') = 18.4 → '18.4'.

    LawinServer's GetVersionInfo does memory.build = Number(Build) and looks
    up injected[memory.build] — JS coerces the number to the string key.
    """
    m = re.search(r"Release-(\d+)\.(\d+)", build_version)
    if not m:
        return None
    b = f"{m.group(1)}.{m.group(2)}"
    # JS Number(): "18.40" → 18.4, "13.40" → 13.4, "12.61" → 12.61
    n = float(b)
    # format without trailing zeros like JS
    key = repr(n)
    return key


def main():
    apk_path, lawin_dir = sys.argv[1], sys.argv[2]

    z = zipfile.ZipFile(apk_path)
    cc = None
    for name in z.namelist():
        if name.endswith("cloudcontent.json"):
            cc = json.loads(z.read(name))
            break
    if not cc:
        print("ERROR: no cloudcontent.json in APK")
        return 1

    app_name = cc.get("AppName", "FortniteContentBuilds")
    build_version = cc.get("BuildVersion", "")
    manifest_path = cc.get("ManifestPath", "")
    manifest_hash = cc.get("ManifestHash", "")

    key = version_key(build_version)
    if not key:
        print(f"ERROR: cannot parse version from {build_version!r}")
        return 1

    # exactly the entry OGFN's getInstalledBuilds writes (labelName and
    # signature = BACKEND_DIR = "LawinServer", distribution = akamaized)
    entry = {
        "appName": app_name,
        "labelName": "LawinServer",
        "buildVersion": f"{build_version}-Android",
        "catalogItemId": "5cb97847cee34581afdbc445400e2f77",
        "expires": "9999-12-31T23:59:59.999Z",
        "items": {
            "MANIFEST": {
                "signature": "LawinServer",
                "distribution": "https://epicgames-download1.akamaized.net/",
                "path": manifest_path,
                "hash": manifest_hash,
                "additionalDistributions": [],
            }
        },
        "assetId": app_name,
    }

    import os
    cfg_dir = os.path.join(lawin_dir, "Config")
    os.makedirs(cfg_dir, exist_ok=True)
    out = os.path.join(cfg_dir, "injected_manifests.json")
    existing = {}
    if os.path.exists(out):
        try:
            existing = json.load(open(out))
        except Exception:
            existing = {}
    existing[key] = entry
    json.dump(existing, open(out, "w"), indent=2)

    print(f"[manifest] {build_version} → key '{key}'")
    print(f"[manifest] path: {manifest_path}")
    print(f"[manifest] wrote {out} ({len(existing)} build(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
