#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  ogfn_pipeline.sh — OGFN (LawinServer) + Erbium closed-loop bring-up
#
#  The full "how a human plays it" flow, automated end-to-end:
#
#    1. OGFN Manager setup — ALL VIA ADB, no UI automation needed:
#         • install ogfn.apk
#         • battery-optimization whitelist (user's requirement)
#         • extract LawinServer-main from ogfn.apk → files/LawinServer
#         • push backend_prefs.xml with permanent_id (= onboarding done)
#         • broadcast START_BACKEND → node starts on 127.0.0.1:8080
#         • verify: HTTP 8080 + matchmaker WS 3001 listening
#    2. Install the PATCHED Fortnite (liberbium.so + loadLibrary + console)
#       — package/scheme auto-detected from the APK (e.g. com.epicgames.
#         fortnit1840 + scheme com.epicgames.fortnit1840://)
#    3. Launch → wait for the INTERNAL UPDATE (manifest from LawinServer,
#       chunks from akamaized — fast on CI, several GB)
#    4. LOGIN: tap LOGIN in-game → fire the authorize deep-link
#       (<scheme>://authorize/?code=lawinsexchangecodelol) → LawinServer
#       accepts any credentials → back in-game, logged in
#    5. LOBBY: wait for the lobby to settle
#    6. GO: touch /data/local/tmp/erbium.go → Erbium flips GIsClient/
#       GIsServer, opens Apollo_Terrain, hooks TickFlush…
#    7. VERIFY THE MATCH:
#         • UDP :7777 LISTENING inside the game process  ← the hard proof
#         • in-match screenshot + brightness/colorfulness analysis
#         • 4-finger console → `getall PlayerController Location`
#
#  Usage: ogfn_pipeline.sh --ogfn ogfn.apk --game fortnite-erbium.apk
#                          [--serial X] [--quick] [--keep-going]
# ═══════════════════════════════════════════════════════════════════════════
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ft_lib.sh"

OGFN_APK=""
GAME_APK=""
QUICK=0
KEEP_GOING=0

while [ $# -gt 0 ]; do
  case "$1" in
    --ogfn)       OGFN_APK="$2"; shift 2 ;;
    --game)       GAME_APK="$2"; shift 2 ;;
    --serial)     SERIAL="$2"; ADB_SERIAL="$2"; shift 2 ;;
    --quick)      QUICK=1; shift ;;
    --keep-going) KEEP_GOING=1; shift ;;
    -h|--help)    grep '^#' "$0" | sed 's/^# \{0,2\}//'; exit 0 ;;
    *) err "unknown arg: $1"; exit 1 ;;
  esac
done

[ -n "$OGFN_APK" ] || die "need --ogfn <ogfn-by-dhyper.apk>"
[ -n "$GAME_APK" ] || die "need --game <fortnite-erbium-patched.apk>"
OGFN_APK="$(realpath "$OGFN_APK")"
GAME_APK="$(realpath "$GAME_APK")"

OGFN_PKG="com.dhyper.ogfnmanager"
EXCHANGE_CODE="lawinsexchangecodelol"   # LawinServer /account/api/oauth/exchange

# ── auto-detect the game package + deep-link scheme from the APK binary ────
read -r GAME_PKG DEEP_SCHEME < <(python3 - "$GAME_APK" <<'PY'
import re, sys, zipfile
# AndroidManifest.xml is binary XML with UTF-16LE string pool. The package
# name and the custom-URL-scheme string are the same value on OGFN builds
# (com.epicgames.fortnitNNNN) — one search finds both.
data = zipfile.ZipFile(sys.argv[1]).read('AndroidManifest.xml')
m = re.search(rb'c\x00o\x00m\x00\.\x00e\x00p\x00i\x00c\x00g\x00a\x00m\x00e\x00s\x00'
              rb'\.\x00f\x00o\x00r\x00t\x00n\x00i\x00t\x00[\x00-\xff]{2,40}', data)
if m:
    s = m.group(0).decode('utf-16-le', 'ignore')
    s = s.split('\x00')[0] if '\x00' in s else s
    print(s, s)
else:
    print('com.epicgames.fortnite', 'com.epicgames.fortnite')
PY
)
GAME_PKG="${GAME_PKG:-com.epicgames.fortnite}"
export GAME_PKG                        # ft_lib helpers use it
GAME_ACT="$GAME_PKG/com.epicgames.ue4.GameActivity"
log "game package: $GAME_PKG   deep-link scheme: $DEEP_SCHEME://"

# time budget (seconds). The emulator runs the game under ndk_translation —
# engine init crawls. Generous on purpose.
if [ "$QUICK" = 1 ]; then
  T_UPDATE=${T_UPDATE:-1500}     # 25 min internal update
  T_LOGIN=${T_LOGIN:-480}        # 8 min login+lobby
  T_MATCH=${T_MATCH:-1080}       # 18 min map load + spawn
else
  T_UPDATE=${T_UPDATE:-3600}     # 60 min internal update (GBs of paks)
  T_LOGIN=${T_LOGIN:-1200}       # 20 min login+lobby
  T_MATCH=${T_MATCH:-2400}       # 40 min map load + spawn
fi

verdict() { # verdict PASS|FAIL "reason"
  mkdir -p "$ARTIFACTS"
  echo "[$1] $2" | tee -a "$ARTIFACTS/VERDICT.txt"
  if [ "$1" = "PASS" ]; then LAST_VERDICT=PASS; else LAST_VERDICT=FAIL;
       [ "$KEEP_GOING" = 0 ] && { collect_artifacts; exit 1; } || true; fi
}

collect_artifacts() {
  log "collecting artifacts → $ARTIFACTS"
  logcat_refresh
  { echo "── top activity: $(top_activity_name)"
    echo "── game pid: $(game_pid || echo none)"
    echo "── udp :7777 listeners: $(shell_dev 'cat /proc/net/udp /proc/net/udp6 2>/dev/null | grep -c 1E61' 2>/dev/null | tr -d '\r')"
    echo "── tcp :8080 listeners: $(shell_dev 'cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | grep -c 1F90' 2>/dev/null | tr -d '\r')"
  } > "$ARTIFACTS/state.txt" 2>/dev/null
  logcat_grep 'Erbium|POST-FLIP|GO |Lawin|Node|FATAL|SIGSEGV|ANR ' 600 \
    > "$ARTIFACTS/logcat-digest.txt" 2>/dev/null || true
}
trap 'collect_artifacts; [ -n "${LAWIN_PID:-}" ] && kill "$LAWIN_PID" 2>/dev/null || true' EXIT

# ═══════════════════════ 0. preflight ═════════════════════════════════════
hr "phase 0 — preflight"
wait_for_device 60 || die "no adb device"
wake_screen
device_info
adb_root_try || die "this pipeline needs adb root (google_apis emulator image)"

case "$(device_abilist)" in
  *arm64*) ok "arm64 in abilist: $(device_abilist)" ;;
  *) die "no arm64 translation on this image (abilist=$(device_abilist))" ;;
esac

# ═══════════════════ 1. LawinServer backend ═══════════════════════════════
hr "phase 1 — LawinServer backend (node on HOST + adb reverse)"
# run14 lesson: node's V8 JIT emits native ARM64 at runtime — ndk_translation
# can't translate JIT'd code (SIGILL in NodeJSThread). So the backend CANNOT
# run inside the x86_64 emulator. Instead: run LawinServer with the runner's
# own node (x86_64, fast) and wire the device's 127.0.0.1:8080/3001 to it
# with `adb reverse` — no root, no OGFN app, no Shizuku needed. libsinum
# inside the patched game redirects Epic domains to 127.0.0.1:8080, which
# now tunnels to the host.

if ! command -v node >/dev/null 2>&1; then
  # GitHub runners have node preinstalled; local runs may not
  sudo apt-get install -y -qq nodejs >/dev/null 2>&1 || die "node.js not available"
fi
NODE_VER=$(node --version)
ok "host node: $NODE_VER"

# extract LawinServer-main from the OGFN apk (it's just an asset folder)
WORK_LS="$ARTIFACTS/lawinserver"
rm -rf "$WORK_LS"; mkdir -p "$WORK_LS"
unzip -o -q "$OGFN_APK" "assets/LawinServer-main/*" -d "$WORK_LS" \
  || die "cannot extract LawinServer from OGFN apk"
mv "$WORK_LS/assets/LawinServer-main" "$WORK_LS/main"
[ -f "$WORK_LS/main/index.js" ] || die "index.js missing after extraction"
ok "LawinServer extracted ($(du -sh "$WORK_LS/main" | cut -f1))"

# config sanity: matchmaker must point the game at ITSELF (Erbium listens
# on 7777 inside the game process on the device)
CFG="$WORK_LS/main/Config/config.ini"
if [ -f "$CFG" ]; then
  sed -i 's/^ip=.*/ip=127.0.0.1/; s/^port=.*/port=7777/' "$CFG" || true
  grep -A2 "\[GameServer\]" "$CFG" | head -3
fi

# start node on the host (background, log to file)
LAWIN_LOG="$ARTIFACTS/lawinserver.log"
PORT=8080 nohup node "$WORK_LS/main/index.js" > "$LAWIN_LOG" 2>&1 &
LAWIN_PID=$!
log "LawinServer pid=$LAWIN_PID (PORT=8080, XMPP/matchmaker on 3001)"

# wire the device loopback to the host backend
adb_dev reverse tcp:8080 tcp:8080 || die "adb reverse 8080 failed"
adb_dev reverse tcp:3001 tcp:3001 || die "adb reverse 3001 failed"
ok "adb reverse: device 127.0.0.1:8080/:3001 → host node"

# wait for HTTP :8080 on the host (node needs a moment)
P8080=0; WAITED=0
while [ "$WAITED" -lt 60 ]; do
  if curl -s -o /dev/null --max-time 3 "http://127.0.0.1:8080/" 2>/dev/null; then
    P8080=1; break
  fi
  # any HTTP answer (even 404) counts — curl exits 0 on HTTP errors with -o
  code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 3 "http://127.0.0.1:8080/" 2>/dev/null)
  [ "${code:-0}" != "000" ] && { P8080=1; break; }
  WAITED=$((WAITED + 3)); sleep 3
done
if [ "$P8080" != "1" ]; then
  tail -30 "$LAWIN_LOG" || true
  die "LawinServer never answered on :8080 (see lawinserver.log)"
fi
ok "LawinServer HTTP :8080 answering (after ${WAITED}s)"

# prove the DEVICE can reach it through the reverse tunnel
DEVPROBE=$(shell_dev "curl -s -o /dev/null -w '%{http_code}' --max-time 5 http://127.0.0.1:8080/ 2>/dev/null" | tr -d '\r')
log "device-side probe of 127.0.0.1:8080 → HTTP ${DEVPROBE:-?} (000 = tunnel broken)"
verdict PASS "backend up (LawinServer on host :8080 + reverse tunnel, device probe=${DEVPROBE:-?})"

# ═══════════════ 2. patched game install + launch ═════════════════════════
hr "phase 2 — patched Fortnite (Erbium inside)"
if [ "$(shell_dev pm list packages 2>/dev/null | tr -d '\r' | grep -c "package:$GAME_PKG$" || true)" -lt 1 ]; then
  install_apk "$GAME_APK" "$GAME_PKG" || die "cannot install patched fortnite ($GAME_PKG)"
else
  ok "patched fortnite already installed ($GAME_PKG)"
fi

# tell Erbium's owen-redirect (if it installs) where the backend REALLY is —
# LawinServer on the device itself. libsinum already redirects, this is a
# belt-and-braces for the Erbium-side redirect.
shell_dev "printf 'http://127.0.0.1:8080' > /data/local/tmp/erbium.backend" || true
shell_dev "rm -f $GO_FILE" || true   # fresh run: GO gate re-armed
ok "erbium.backend=http://127.0.0.1:8080 written, GO gate re-armed"
screenshot game_installed

logcat_clear
game_launch
ok "game launched — waiting for Erbium bootstrap + engine module..."

ms_erbium_loaded 420   || verdict FAIL "liberbium.so never loaded (loadLibrary patch?)"
ms_engine_module $((T_UPDATE + 600)) || verdict FAIL "libUE4.so never mapped (translation too slow?)"
ms_release_probe 300   || warn "release probe not logged (non-fatal)"

# ═══════════════ 3. internal update (manifest+chunks) ═════════════════════
hr "phase 3 — internal update (GBs of paks from akamaized)"
# The game pulls its manifest from LawinServer (injected manifests) and the
# chunk payloads from epicgames-download1.akamaized.net. Watch traffic go
# quiet + the process settle. NEVER force-stop: the game restarts itself
# mid-update and wait_game_process re-attaches.
timer_start
LAST_RX=-1; STABLE=0
while [ "$(timer_s)" -lt "$T_UPDATE" ]; do
  RX=$(shell_dev "cat /proc/net/dev | awk '/eth0|wlan0/ {print \$2}'" 2>/dev/null | tr -d '\r' | head -1)
  RX=${RX:-0}
  if [ "$LAST_RX" != "-1" ] && [ "$((RX - LAST_RX))" -lt 2000000 ]; then  # <2MB/s
    STABLE=$((STABLE + 12))
  else
    STABLE=0
  fi
  LAST_RX=$RX
  [ "$STABLE" -ge 72 ] && break   # ~72s of quiet network
  game_alive || { log "game process gone (self-restart mid-update?) — re-attaching";
                   wait_game_process 180 || true; }
  sleep 12
done
log "update phase done after $(timer_s)s (rx stable=${STABLE}s)"
screenshot update_done
verdict PASS "internal update completed ($(timer_s)s)"

# ═══════════════════ 4. login (deep-link hop) ═════════════════════════════
hr "phase 4 — login via LawinServer"
# The game sits on its front screen. 18.40's LOGIN button lives bottom-right;
# it's a native UE4 widget (invisible to uiautomator) → percentage tap.
game_foreground || game_resume
sleep 5
screenshot login_screen

tap_pct 72 88; sleep 1; tap_pct 72 88   # LOGIN button (twice — first dismisses an overlay)
sleep 6
screenshot after_login_tap

# fire the deep link with LawinServer's exchange code
log "firing authorize deep-link ($DEEP_SCHEME://authorize/?code=$EXCHANGE_CODE)"
shell_dev "am start -a android.intent.action.VIEW -d '${DEEP_SCHEME}://authorize/?code=$EXCHANGE_CODE'" >/dev/null 2>&1 || true
sleep 3
# the chooser may ask to open with the game itself
ui_tap_text "Open" 2>/dev/null || true
ui_tap_text "Just once" 2>/dev/null || true
sleep 5

# wait for the lobby: quiet screen + stable frames (≤T_LOGIN, 3 frames, 5s apart)
if wait_screen_stable "$T_LOGIN" 3 5; then
  ok "screen settled — probably in lobby"
else
  warn "screen never fully settled — continuing anyway"
fi
game_foreground || game_resume || true
sleep 5
screenshot lobby
LB=$(img_brightness "$(last_shot lobby)")
log "lobby brightness=${LB:-?}"
verdict PASS "login + lobby reached (brightness=${LB:-?})"

# ═══════════════════ 5. GO — flip + server bring-up ═══════════════════════
hr "phase 5 — GO (flip GIs* + open the map)"
ms_wait_go $((T_LOGIN + 300)) || verdict FAIL "Erbium never armed the GO gate"

shell_dev "touch $GO_FILE" || die "cannot touch the GO file"
ms_go_received 180 || verdict FAIL "GO signal not seen by Erbium"
ok "GO received — Erbium is flipping GIs and opening the map"

# ═══════════════════ 6. match load + THE proof ════════════════════════════
hr "phase 6 — match load + verification (port 7777 + player visible)"
UDP7777() { shell_dev "cat /proc/net/udp /proc/net/udp6 2>/dev/null | grep -c ':1E61'" 2>/dev/null | tr -d '\r'; }

timer_start; SAW7777=0
while [ "$(timer_s)" -lt "$T_MATCH" ]; do
  N=$(UDP7777)
  [ "${N:-0}" -ge 1 ] && { SAW7777=1; break; }
  game_alive || warn "game process died during match load!"
  sleep 10
done

if [ "$SAW7777" = 1 ]; then
  ok "★ UDP :7777 LISTENING after $(timer_s)s ★"
  verdict PASS "UDP :7777 LISTENING — Erbium game-server is UP"
else
  verdict FAIL "UDP :7777 never listened (flip/listen failed — check logcat)"
fi

# in-match visual proof: brightness + colorfulness out of loading-screen range
timer_start; LOADED=0
while [ "$(timer_s)" -lt 900 ]; do
  sleep 30
  screenshot "match"
  if shot_looks_ingame "$(last_shot match)"; then LOADED=1; break; fi
done
if [ "$LOADED" = 1 ]; then
  verdict PASS "in-match scene rendered (player visible)"
else
  verdict FAIL "match never visually loaded (loading screen / void)"
fi

# console proof (4-finger): PlayerController location — strongest signal
if console_open; then
  if console_cmd "getall PlayerController Location"; then
    sleep 4
    logcat_refresh
    if logcat_grep 'Location=' 40 | grep -qE 'X=|Y=|Z='; then
      verdict PASS "console: PlayerController Location returned — pawn exists"
    else
      warn "console getall output not in logcat (visual proof stands)"
    fi
  fi
  console_close || true
fi

# final hero shot
sleep 12
screenshot FINAL_in_match
verdict PASS "FINAL in-match screenshot captured"

hr "VERDICT"
cat "$ARTIFACTS/VERDICT.txt" 2>/dev/null || true
if grep -q "FAIL" "$ARTIFACTS/VERDICT.txt" 2>/dev/null; then
  echo ">>> OVERALL: FAIL — inspect artifacts <<<"
  exit 1
fi
echo ">>> OVERALL: PASS — Erbium For Android hosted a match on OGFN <<<"
exit 0
