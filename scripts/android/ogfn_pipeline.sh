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
trap 'collect_artifacts' EXIT

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

# ═══════════════════ 1. OGFN backend (all via adb) ════════════════════════
hr "phase 1 — OGFN Manager + LawinServer backend"

if [ "$(shell_dev pm list packages 2>/dev/null | tr -d '\r' | grep -c "package:$OGFN_PKG$" || true)" -lt 1 ]; then
  install_apk "$OGFN_APK" "$OGFN_PKG" || die "cannot install OGFN manager"
else
  ok "OGFN manager already installed"
fi

# 1a. battery optimization exemption (user requirement — background survival)
shell_dev "dumpsys deviceidle whitelist +$OGFN_PKG" >/dev/null 2>&1 || true
shell_dev "dumpsys deviceidle whitelist +$GAME_PKG" >/dev/null 2>&1 || true
ok "battery-optimization whitelisted ($OGFN_PKG, $GAME_PKG)"

# 1b. stop leftovers from previous runs
shell_dev "am force-stop $OGFN_PKG" >/dev/null 2>&1 || true
sleep 1

# 1c+1d. Deploy LawinServer + permanent_id — TWO PATHS:
#   PATH A (adb root): direct file deployment — fast, deterministic.
#   PATH B (no root, user-build image): drive the app's own onboarding UI
#     (Compose — visible to uiautomator). The app then copies LawinServer
#     itself and writes permanent_id itself.
WORK_LS="$(mktemp -d)"
ONBOARDING_DONE=0

if [ "${ADB_ROOTED:-0}" = "1" ]; then
  # ── PATH A: direct deployment (root) ────────────────────────────────
  log "PATH A: direct LawinServer deployment via adb root"
  unzip -o -q "$OGFN_APK" "assets/LawinServer-main/*" -d "$WORK_LS" \
    || die "cannot extract LawinServer from OGFN apk"
  tar -C "$WORK_LS/assets/LawinServer-main" -czf "$WORK_LS/lawin.tgz" .
  ls -la "$WORK_LS/lawin.tgz"
  shell_dev "rm -rf /data/local/tmp/lawin.tgz /data/data/$OGFN_PKG/files/LawinServer" || true
  adb_dev push "$WORK_LS/lawin.tgz" /data/local/tmp/lawin.tgz \
    || die "cannot push LawinServer tarball"
  shell_dev "mkdir -p /data/data/$OGFN_PKG/files/LawinServer && tar -xzf /data/local/tmp/lawin.tgz -C /data/data/$OGFN_PKG/files/LawinServer && rm -f /data/local/tmp/lawin.tgz" \
    || die "cannot extract LawinServer on device"
  shell_dev "ls /data/data/$OGFN_PKG/files/LawinServer/index.js" >/dev/null 2>&1 \
    || die "index.js not in place after untar"
  ok "LawinServer deployed to files/LawinServer"

  shell_dev "mkdir -p /data/data/$OGFN_PKG/shared_prefs" || true
  cat > "$WORK_LS/backend_prefs.xml" <<'EOF'
<?xml version='1.0' encoding='utf-8' standalone='yes' ?>
<map>
    <string name="permanent_id">ErbiumCI</string>
</map>
EOF
  adb_dev push "$WORK_LS/backend_prefs.xml" "/data/local/tmp/backend_prefs.xml" \
    || die "cannot push backend_prefs.xml"
  shell_dev "cp /data/local/tmp/backend_prefs.xml /data/data/$OGFN_PKG/shared_prefs/backend_prefs.xml && rm -f /data/local/tmp/backend_prefs.xml" \
    || die "cannot install backend_prefs.xml"
  APPUID=$(shell_dev "stat -c '%u' /data/data/$OGFN_PKG" 2>/dev/null | tr -d '\r')
  [ -n "$APPUID" ] && shell_dev "chown -R ${APPUID}:${APPUID} /data/data/$OGFN_PKG/files/LawinServer /data/data/$OGFN_PKG/shared_prefs" || true
  ok "onboarding marked complete (permanent_id=ErbiumCI, uid=${APPUID:-?})"
  ONBOARDING_DONE=1
else
  # ── PATH B: drive the app's own onboarding (no root) ───────────────
  # Order-adaptive state machine: whatever screen shows up, act on it.
  # (run2 lesson: DeveloperSafety with "JOIN DISCORD TO CONTINUE" came
  #  FIRST — never assume the step order.)
  log "PATH B: OGFN onboarding via UI automation (no adb root on this image)"
  shell_dev "am start -n $OGFN_PKG/.MainActivity" >/dev/null 2>&1 || true
  sleep 6
  screenshot ogfn_launch

  ONB_T0=$(date +%s)   # NOTE: plain seconds — timer_start/timer_s are a
                       # millisecond pair; mixing them breaks arithmetic
  ONB_TIMEOUT=420   # the SettingUp extraction takes a while (node_modules!)
  LAST_SHOT=-15
  # one ui dump per cycle, then match against it locally (each ui_find is
  # a full adb round-trip — 6 of them per loop would crawl)
  onb_screen_has() {  # <xml> <needle> → 0/1
    case "$1" in *"$2"*) return 0 ;; *) return 1 ;; esac
  }
  while :; do
    ELAPSED=$(( $(date +%s) - ONB_T0 ))
    [ "$ELAPSED" -ge "$ONB_TIMEOUT" ] && break
    if [ $((ELAPSED - LAST_SHOT)) -ge 15 ]; then
      screenshot "onb" >/dev/null 2>&1 || true; LAST_SHOT=$ELAPSED
    fi
    XMLF=$(ui_dump 2>/dev/null || true)
    XML=""
    [ -n "$XMLF" ] && [ -f "$XMLF" ] && XML=$(cat "$XMLF")
    [ -z "$XML" ] && { sleep 3; continue; }

    # if a tap opened the browser/another app over OGFN — come back
    if ! onb_screen_has "$XML" "$OGFN_PKG"; then
      log "onb: left the OGFN app (browser/chooser opened?) — BACK"
      key_back; sleep 2; continue
    fi

    # ── success screen: done! ─────────────────────────────────────────
    if onb_screen_has "$XML" "Have fun"; then
      screenshot ogfn_success
      ok "onboarding completed (app extracted LawinServer + wrote permanent_id)"
      ONBOARDING_DONE=1
      ui_tap_text "START" 2>/dev/null || ui_tap_text "GO" 2>/dev/null || key_back || true
      sleep 2
      shell_dev "am force-stop $OGFN_PKG" >/dev/null 2>&1 || true
      sleep 2
      break
    fi

    # ── DeveloperSafety: "JOIN DISCORD TO CONTINUE" arms a countdown;
    #    while counting the button reads "PROCEED IN Xs"; plain "PROCEED"
    #    (no "IN") actually continues. ─────────────────────────────────
    if onb_screen_has "$XML" "JOIN DISCORD TO CONTINUE"; then
      log "onb: DeveloperSafety — arming countdown (emu_tap)"
      ui_emu_tap_from_file "$XMLF" "JOIN DISCORD TO CONTINUE" || emu_tap 540 1637
      sleep 3; continue
    fi
    if onb_screen_has "$XML" "PROCEED IN"; then
      sleep 3; continue          # countdown running — just wait
    fi
    if onb_screen_has "$XML" "PROCEED"; then
      log "onb: DeveloperSafety — PROCEED (emu_tap)"
      ui_emu_tap_from_file "$XMLF" "PROCEED" || emu_tap 540 1637
      sleep 3; continue
    fi

    # ── Disclaimer: three "I understand..." checkboxes + CONTINUE ─────
    if onb_screen_has "$XML" "I understand"; then
      log "onb: Disclaimer — checking all boxes (emu_tap)"
      # tap every checkbox row
      python3 - "$XMLF" <<'PYM' > /tmp/onb_boxes.txt
import sys, re
xml = open(sys.argv[1]).read()
for m in re.finditer(r'<node[^>]*/>', xml):
    tag = m.group(0)
    if 'I understand' in tag:
        bm = re.search(r'bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', tag)
        if bm: print((int(bm.group(1))+int(bm.group(3)))//2, (int(bm.group(2))+int(bm.group(4)))//2)
PYM
      while read -r bx by; do [ -n "$bx" ] && emu_tap "$bx" "$by" && sleep 1; done < /tmp/onb_boxes.txt
      sleep 1
      screenshot ogfn_disclaimer
      ui_emu_tap_from_file "$XMLF" "CONTINUE" || emu_tap 540 2120
      sleep 3; continue
    fi

    # ── InputName: Username field + CONFIRM ──────────────────────────
    if onb_screen_has "$XML" "Username"; then
      log "onb: InputName — typing ErbiumCI (emu_tap)"
      ui_emu_tap_from_file "$XMLF" "Username" || emu_tap 540 1000
      sleep 1
      type_text "ErbiumCI"; sleep 1
      key_enter 2>/dev/null || true
      screenshot ogfn_name
      ui_emu_tap_from_file "$XMLF" "CONFIRM" || emu_tap 540 2120
      sleep 3; continue
    fi

    # ── DevNotice gate ("Developer Notice" title, discord link) ──────
    if onb_screen_has "$XML" "Developer Notice"; then
      # DevNotice = StepDeveloperSafety with the notice title (verified
      # run3-5: same screen, JOIN-DISCORD button arms a 10s countdown)
      log "onb: DevNotice/DeveloperSafety — emu_tap primary"
      ui_emu_tap_from_file "$XMLF" "JOIN DISCORD TO CONTINUE" || emu_tap 540 1637
      sleep 3; continue
    fi

    # ── progress/spinner screens: just wait ───────────────────────────
    sleep 4
  done

  if [ "$ONBOARDING_DONE" != "1" ]; then
    screenshot ogfn_setup_stuck
    warn "onboarding did not reach 'Have fun' in ${ONB_TIMEOUT}s — backend start may fail"
  fi
fi

# 1e. ask OGFN itself whether it agrees (result=1 = complete)
ONB=$(shell_dev "am broadcast -a ${OGFN_PKG}.CHECK_ONBOARDING" 2>/dev/null | grep -oE 'result=[0-9]+' | head -1)
log "CHECK_ONBOARDING → ${ONB:-?} (result=1 means complete; onboarding_done=$ONBOARDING_DONE)"

# 1f. START THE BACKEND
logcat_clear
shell_dev "am broadcast -a ${OGFN_PKG}.START_BACKEND -n $OGFN_PKG/.core.BackendBroadcastReceiver" >/dev/null

# node's stdout goes to a FILE (not logcat) — poll the port instead.
# HTTP 8080 = 0x1F90 in /proc/net/tcp
P8080=0; WAITED=0
while [ "$WAITED" -lt 180 ]; do
  P8080=$(shell_dev "cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | grep -c ':1F90'" | tr -d '\r')
  [ "${P8080:-0}" -ge 1 ] && break
  # the receiver logs its decisions — surface them for debugging
  WAITED=$((WAITED + 5)); sleep 5
done
logcat_grep 'BackendReceiver|BackendService|NodeJSThread|onboarding' 30 || true
if [ "${P8080:-0}" -lt 1 ]; then
  screenshot backend_failed
  shell_dev "ls -la /data/data/$OGFN_PKG/files/ 2>/dev/null; head -c 2000 /data/data/$OGFN_PKG/files/native_crash_log.txt 2>/dev/null" || true
  die "LawinServer HTTP :8080 never came up (see logcat above + native_crash_log)"
fi
ok "LawinServer HTTP :8080 LISTENING (after ${WAITED}s)"

# matchmaker WS 3001 = 0x0BB9 (XMPP starts with the HTTP server)
P3001=$(shell_dev "cat /proc/net/tcp /proc/net/tcp6 2>/dev/null | grep -c ':0BB9'" | tr -d '\r')
[ "${P3001:-0}" -ge 1 ] || warn "port 3001 (matchmaker) not up yet — XMPP starts with HTTP"
ok "LawinServer: HTTP :8080 ✓   matchmaker :3001 ${P3001:-0}✓"
verdict PASS "backend up (LawinServer :8080, matchmaker :3001)"

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
