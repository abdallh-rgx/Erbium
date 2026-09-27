#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  ft_lib.sh — Fortnite-on-Android automation library (adb-driven)
#
#  Source this file, never execute it directly:
#      source "$(dirname "$0")/ft_lib.sh"
#
#  Design rules (learned the hard way):
#    * NEVER force-stop the game — "exit without a full exit". The game must
#      survive internal updates, self-restarts and the browser login hop.
#      Background = HOME key, foreground = am start (resumes the task).
#    * Wait for INTERNAL LOADING to COMPLETE before declaring success — a
#      screenshot taken on the loading screen or before the pawn spawns does
#      not count. "Player missing / under the map" = FAIL, not PASS.
#    * Every wait has a timeout, every failure says WHY and WHAT to check.
#    * Everything lands in $ARTIFACTS so a failed run is debuggable.
# ═══════════════════════════════════════════════════════════════════════════
# shellcheck shell=bash

# ─────────────────────────── configuration ─────────────────────────────────
: "${GAME_PKG:=com.epicgames.fortnite}"
: "${GAME_ACT:=com.epicgames.unreal/GameActivity}"
: "${ADB_SERIAL:=}"            # empty = first device; else -s <serial>
: "${BACKEND_URL:=http://127.0.0.1:3551}"   # URL the DEVICE sees (emulator: http://10.0.2.2:3551)
: "${LOGIN_USER:=ErbiumTester}"
: "${ARTIFACTS:=$PWD/artifacts}"
: "${SHOTS_DIR:=$ARTIFACTS/shots}"
: "${UI_DIR:=$ARTIFACTS/ui}"
: "${LOGCAT_FILE:=$ARTIFACTS/logcat-full.txt}"
: "${GO_FILE:=/data/local/tmp/erbium.go}"
: "${MT_HOLD_MS:=400}"         # how long the 4 fingers stay down
: "${SCREENSHOT_POLL_MS:=2000}"

# derived
GAME_ACT_FQ="${GAME_ACT#*/}"   # com.epicgames.unreal.GameActivity
mkdir -p "$ARTIFACTS" "$SHOTS_DIR" "$UI_DIR" 2>/dev/null || true

# ─────────────────────────── core utilities ────────────────────────────────
_now_ms()  { date +%s%3N; }
_now_s()   { date +%s; }
timer_start() { TIMER_T0=$(_now_ms); }
timer_ms()   { echo $(( $(_now_ms) - TIMER_T0 )); }
timer_s()    { echo $(( ( $(_now_ms) - TIMER_T0 ) / 1000 )); }

_ts()      { date +%H%M%S; }
log()   { printf '\033[36m[%s]\033[0m %s\n'  "$(date +%T)" "$*"; }
ok()    { printf '\033[32m[%s] ✔ %s\033[0m\n' "$(date +%T)" "$*"; }
warn()  { printf '\033[33m[%s] ⚠ %s\033[0m\n' "$(date +%T)" "$*"; }
err()   { printf '\033[31m[%s] ✘ %s\033[0m\n' "$(date +%T)" "$*" >&2; }
die()   { err "FATAL: $*"; collect_artifacts "die"; exit 1; }
hr()    { log "──────────────────────── $* ────────────────────────"; }

# run a hook before dying (set by the driver)
collect_artifacts() { :; }

# retry <tries> <sleep_s> <cmd...>  — 0 tries = forever
retry() {
  local tries=$1 sleep_s=$2; shift 2
  local n=0
  until "$@"; do
    n=$((n+1))
    if [ "$tries" != "0" ] && [ "$n" -ge "$tries" ]; then
      err "retry: gave up after $n tries: $*"
      return 1
    fi
    sleep "$sleep_s"
  done
}

# ─────────────────────────── adb plumbing ──────────────────────────────────
adb_dev() {
  if [ -n "$ADB_SERIAL" ]; then adb -s "$ADB_SERIAL" "$@"; else adb "$@"; fi
}

shell_dev() {  # shell_dev "cmd" — one call, proper quoting
  adb_dev shell "$@"
}

wait_for_device() {  # [timeout_s]
  local timeout=${1:-120} t0=$(_now_s)
  log "waiting for adb device (≤${timeout}s)..."
  until adb_dev get-state >/dev/null 2>&1; do
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "no adb device came online"; return 1; }
    sleep 2
  done
  ok "device online: $(adb_dev get-state)"
}

is_emulator() { [ "$(shell_dev getprop ro.kernel.qemu 2>/dev/null | tr -d '\r')" = "1" ]; }

device_sdk() { shell_dev getprop ro.build.version.sdk 2>/dev/null | tr -d '\r'; }
device_abilist() { shell_dev getprop ro.product.cpu.abilist 2>/dev/null | tr -d '\r'; }
device_prop() { shell_dev getprop "$1" 2>/dev/null | tr -d '\r'; }

device_info() {
  hr "device info"
  log "model=$(device_prop ro.product.model) android=$(device_prop ro.build.version.release) sdk=$(device_sdk)"
  log "abilist=$(device_abilist) emulator=$(is_emulator && echo yes || echo no)"
  log "screen=$(shell_dev wm size | tr -d '\r' | head -1)"
}

# root access if the image allows it (google_apis emulators do; playstore
# builds don't — everything here must also work unrooted)
adb_root_try() {
  if [ "$(adb_dev shell id 2>/dev/null | tr -d '\r')" = "uid=0(root)" ]; then
    ADB_ROOTED=1; return 0
  fi
  adb_dev root >/dev/null 2>&1
  sleep 2
  if [ "$(adb_dev shell id 2>/dev/null | tr -d '\r')" = "uid=0(root)" ]; then
    ADB_ROOTED=1; ok "adb root enabled"
  else
    ADB_ROOTED=0; warn "adb root unavailable (playstore image / user build) — multitouch falls back to emulator console"
  fi
}

screen_size() {  # → "W H"
  shell_dev wm size 2>/dev/null | grep -oE '[0-9]+x[0-9]+' | head -1 | tr 'x' ' '
}
screen_w() { screen_size | cut -d' ' -f1; }
screen_h() { screen_size | cut -d' ' -f2; }

# ─────────────────────────── logcat ────────────────────────────────────────
logcat_clear() { adb_dev logcat -c 2>/dev/null || true; LOGCAT_LINE0=$(logcat_linecount); }

logcat_linecount() {
  [ -f "$LOGCAT_FILE" ] && wc -l < "$LOGCAT_FILE" || echo 0
}

logcat_refresh() {  # pull everything so far into $LOGCAT_FILE (append)
  adb_dev logcat -d -v time >> "$LOGCAT_FILE" 2>/dev/null || true
}

# logcat_count "pattern" — occurrences so far (refresh first)
logcat_count() {
  logcat_refresh
  local n=0
  [ -f "$LOGCAT_FILE" ] && n=$(grep -c -E "$1" "$LOGCAT_FILE" 2>/dev/null)
  echo "${n:-0}"
}

# logcat_grep "pattern" [lines]
logcat_grep() { logcat_refresh; grep -E "$1" "$LOGCAT_FILE" 2>/dev/null | tail "${2:-20}"; }

# logcat_wait "pattern" [timeout_s] [label] — poll until pattern appears
logcat_wait() {
  local pattern=$1 timeout=${2:-120} label=${3:-$1}
  local t0=$(_now_s)
  log "logcat: waiting for '$label' (≤${timeout}s)..."
  while :; do
    if [ "$(logcat_count "$pattern")" -gt 0 ] 2>/dev/null; then
      ok "logcat: '$label' seen (+$(( $(_now_s) - t0 ))s)"
      return 0
    fi
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "logcat: '$label' NOT seen within ${timeout}s"; return 1; }
    sleep 3
  done
}

has_pattern() { logcat_refresh; grep -q -E "$1" "$LOGCAT_FILE" 2>/dev/null; }

# crash detection: native tombstone / java FATAL involving the game
crash_detected() {
  has_pattern 'FATAL EXCEPTION|SIGSEGV|SIGABRT|Force finishing activity '"$GAME_PKG" && return 0
  adb_dev shell ls /data/tombstones 2>/dev/null | grep -q tombstone
}

# ─────────────────────────── app lifecycle ─────────────────────────────────
# RULE: never `am force-stop` / `force-stop` the game. Only am start / HOME.

game_pid() { shell_dev pidof "$GAME_PKG" 2>/dev/null | tr -d '\r' | awk '{print $1}'; }
game_alive() { [ -n "$(game_pid)" ]; }

game_launch() {  # cold start (only at the very beginning of a run)
  log "launching $GAME_PKG..."
  shell_dev monkey -p "$GAME_PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
}

game_resume() {  # bring the EXISTING task to front — never a cold restart
  if ! game_alive; then
    warn "game not running — game_resume falls back to cold launch"
    game_launch
    return
  fi
  # Launching the launcher intent while the task exists = standard Android
  # app resume: the task comes to the front with its current activity. It
  # does NOT recreate the task or start a new splash on top.
  log "resuming game task (no restart)..."
  shell_dev monkey -p "$GAME_PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
}

game_background() {  # background WITHOUT killing — "exit without full exit"
  key 3   # KEYCODE_HOME
}

top_activity() {
  shell_dev dumpsys window 2>/dev/null | grep -E 'mCurrentFocus|mFocusedApp' | head -2 | tr -d '\r'
}

top_activity_name() {
  top_activity | grep -oE '[a-zA-Z0-9._]+/[a-zA-Z0-9._]+' | head -1
}

game_foreground() {
  local t; t=$(top_activity_name)
  [ -n "$t" ] && [[ "$t" == *"$GAME_PKG"* ]]
}

wait_game_process() {  # [timeout_s] — handles the self-restart after updates
  local timeout=${1:-300} t0=$(_now_s)
  log "waiting for $GAME_PKG process (≤${timeout}s)..."
  while :; do
    game_alive && { ok "game process up (pid $(game_pid))"; return 0; }
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "game process never appeared"; return 1; }
    sleep 3
  done
}

wait_game_foreground() {  # [timeout_s]
  local timeout=${1:-180} t0=$(_now_s)
  log "waiting for game in foreground (≤${timeout}s)..."
  while :; do
    game_foreground && { ok "game is foreground: $(top_activity_name)"; return 0; }
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "game never reached foreground — focus: $(top_activity_name)"; return 1; }
    sleep 3
  done
}

wait_activity() {  # "ActivityName" [timeout_s] — any activity containing name
  local name=$1 timeout=${2:-180} t0=$(_now_s)
  log "waiting for activity *$name* (≤${timeout}s)..."
  while :; do
    local t; t=$(top_activity_name)
    [ -n "$t" ] && [[ "$t" == *"$name"* ]] && { ok "activity: $t"; return 0; }
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "activity *$name* never focused (got: $t)"; return 1; }
    sleep 3
  done
}

is_installed() { shell_dev pm list packages 2>/dev/null | tr -d '\r' | grep -q "package:$GAME_PKG$"; }

install_apk() {  # <apk>
  local apk=$1
  [ -f "$apk" ] || { err "APK not found: $apk"; return 1; }
  log "installing $apk ..."
  adb_dev install -r -t "$apk" 2>&1 | tee -a "$ARTIFACTS/install.log" | tail -2
  is_installed || { err "install failed — see $ARTIFACTS/install.log"; return 1; }
  ok "installed"
}

# ─────────────────────────── input ─────────────────────────────────────────
key()      { shell_dev input keyevent "$1"; }            # key 3 = HOME, 4 = BACK
key_back() { key 4; }
key_home() { key 3; }
key_enter(){ key 66; }
key_esc()  { key 111; }
key_del()  { key 67; }

tap()       { shell_dev input tap "$1" "$2"; }            # tap <x> <y>
tap_pct()   { # tap_pct <xp%> <yp%>
  local w h; read -r w h <<< "$(screen_size)"
  shell_dev input tap $(( w * $1 / 100 )) $(( h * $2 / 100 ))
}
swipe()     { shell_dev input swipe "$1" "$2" "$3" "$4" "${5:-300}"; }
swipe_up()  { # unlock gesture
  local w h; read -r w h <<< "$(screen_size)"
  shell_dev input swipe $(( w / 2 )) $(( h * 72 / 100 )) $(( w / 2 )) $(( h * 25 / 100 )) 250
}

wake_screen() {
  shell_dev input keyevent KEYCODE_WAKEUP 2>/dev/null || key 224
  sleep 1
  # if lockscreen: try the standard swipe-up
  local f; f=$(top_activity_name)
  [[ "$f" == *Keyguard* || "$f" == *Launcher* && -z "$f" ]] && swipe_up
  sleep 1
}

# type_text "with spaces" — `input text` mangles spaces; go word by word
type_text() {
  local text=$1 w="" first=1
  for w in $text; do
    [ $first -eq 0 ] && shell_dev input keyevent 62   # SPACE
    first=0
    # escape special chars for input text
    shell_dev input text "${w//%/\\%}"
  done
}

# ─────────────────────────── multitouch (4-finger console) ─────────────────
# The injected GameActivity.dispatchTouchEvent fires the console when it sees
# exactly 4 pointers. Three injection backends, first that works wins:

find_touchscreen() {  # → "/dev/input/eventN" with ABS_MT_POSITION_X
  shell_dev getevent -pl 2>/dev/null | awk '
    /^add device/ { dev=$4 }
    /ABS_MT_POSITION_X/ { print dev; exit }'
}

# getevent read-back: listen N seconds for events on a device
_touch_events_seen() {  # <device> <seconds>
  timeout "$2" adb_dev shell "timeout $2 getevent -c 1 $1" >/dev/null 2>&1
}

mt_sendevent_fingers() {  # <dev> <x1> <y1> <x2> <y2> <x3> <y3> <x4> <y4>
  local dev=$1; shift
  local xs=("$1" "$3" "$5" "$7") ys=("$2" "$4" "$6" "$8")
  local baseid=$(( 300 + RANDOM % 500 ))
  local ev="sendevent $dev 3 47 0; sendevent $dev 3 57 $((baseid));
            sendevent $dev 3 53 ${xs[0]}; sendevent $dev 3 54 ${ys[0]};"
  for i in 1 2 3; do
    ev="$ev sendevent $dev 3 47 $i; sendevent $dev 3 57 $((baseid+i));
        sendevent $dev 3 53 ${xs[$i]}; sendevent $dev 3 54 ${ys[$i]};"
  done
  ev="$ev sendevent $dev 0 0 0"     # SYN_REPORT → 4 pointers DOWN
  local hold=$(( MT_HOLD_MS / 100 )); local h=0
  while [ $h -lt $hold ]; do ev="$ev sendevent $dev 0 0 0"; h=$((h+1)); done
  for i in 0 1 2 3; do ev="$ev sendevent $dev 3 47 $i; sendevent $dev 3 57 4294967295;"; done
  ev="$ev sendevent $dev 0 0 0"     # SYN_REPORT → all UP
  shell_dev "$ev" >/dev/null 2>&1
}

mt_emu_fingers() {  # <x1> <y1> <x2> <y2> <x3> <y3> <x4> <y4> — emulator console
  local xs=("$1" "$3" "$5" "$7") ys=("$2" "$4" "$6" "$8")
  local args=()
  for i in 0 1 2 3; do
    args+=(EV_ABS:ABS_MT_SLOT:$i EV_ABS:ABS_MT_TRACKING_ID:$((400+i)) \
           EV_ABS:ABS_MT_POSITION_X:${xs[$i]} EV_ABS:ABS_MT_POSITION_Y:${ys[$i]})
  done
  args+=(EV_SYN:SYN_REPORT:0)
  adb_dev emu event send "${args[@]}" >/dev/null 2>&1
  sleep 0.$(( MT_HOLD_MS % 1000 ))
  adb_dev emu event send EV_SYN:SYN_REPORT:0 >/dev/null 2>&1
  # releases (tracking id -1 = 4294967295)
  local rel=()
  for i in 0 1 2 3; do rel+=(EV_ABS:ABS_MT_SLOT:$i EV_ABS:ABS_MT_TRACKING_ID:4294967295); done
  rel+=(EV_SYN:SYN_REPORT:0)
  adb_dev emu event send "${rel[@]}" >/dev/null 2>&1
}

four_finger_tap() {  # → opens the in-game console (4 fingers at once)
  local w h; read -r w h <<< "$(screen_size)"
  [ -n "$w" ] || { err "four_finger_tap: no screen size"; return 1; }
  # spread the 4 fingers across the middle of the screen, away from buttons
  local x1=$(( w * 20 / 100 )) y1=$(( h * 35 / 100 ))
  local x2=$(( w * 80 / 100 )) y2=$(( h * 35 / 100 ))
  local x3=$(( w * 20 / 100 )) y3=$(( h * 65 / 100 ))
  local x4=$(( w * 80 / 100 )) y4=$(( h * 65 / 100 ))

  if [ "${ADB_ROOTED:-0}" = "1" ] && is_emulator; then
    local dev; dev=$(find_touchscreen)
    if [ -n "$dev" ]; then
      log "4-finger: sendevent on $dev"
      mt_sendevent_fingers "$dev" "$x1" "$y1" "$x2" "$y2" "$x3" "$y3" "$x4" "$y4" && return 0
    fi
  fi
  if is_emulator; then
    log "4-finger: emulator console events"
    mt_emu_fingers "$x1" "$y1" "$x2" "$y2" "$x3" "$y3" "$x4" "$y4" && return 0
  fi
  if [ "${ADB_ROOTED:-0}" = "1" ]; then
    local dev; dev=$(find_touchscreen)
    if [ -n "$dev" ]; then
      log "4-finger: sendevent (rooted) on $dev"
      mt_sendevent_fingers "$dev" "$x1" "$y1" "$x2" "$y2" "$x3" "$y3" "$x4" "$y4" && return 0
    fi
  fi
  err "4-finger: no working multitouch backend (need emulator or adb root) — tap the screen with 4 real fingers"
  return 1
}

# ─────────────────────────── screenshots ───────────────────────────────────
screenshot() {  # <name> → prints local path
  local name=${1:-shot}
  local remote=/sdcard/ft_${name}_$(_ts).png
  local local_f="$SHOTS_DIR/${name}_$(_ts).png"
  shell_dev screencap -p "$remote" 2>/dev/null || { err "screencap failed"; return 1; }
  adb_dev pull "$remote" "$local_f" >/dev/null 2>&1 || { err "pull $remote failed"; return 1; }
  shell_dev rm -f "$remote" 2>/dev/null
  echo "$local_f"
}

# last screenshot of a given name prefix (for later analysis)
last_shot() { ls -t "$SHOTS_DIR"/${1}_*.png 2>/dev/null | head -1; }

# ─────────────────────────── image analysis (python) ───────────────────────
# img_stats <png> → "brightness colorfulness"  (0-255 each)
img_stats() {
  python3 - "$1" <<'PY'
import sys
try:
    from PIL import Image
except ImportError:
    print("-1 -1"); sys.exit(0)
try:
    im = Image.open(sys.argv[1]).convert("RGB").resize((96, 96))
    px = list(im.getdata())
    n = len(px)
    brightness = sum((r*299 + g*587 + b*114) // 1000 for r, g, b in px) / n
    mean = [sum(c[i] for c in px) / n for i in range(3)]
    var = [sum((c[i] - mean[i]) ** 2 for c in px) / n for i in range(3)]
    colorfulness = (var[0] + var[1] + var[2]) ** 0.5
    print(f"{brightness:.1f} {min(colorfulness, 255):.1f}")
except Exception:
    print("-1 -1")
PY
}
img_brightness()  { img_stats "$1" | cut -d' ' -f1; }
img_colorfulness(){ img_stats "$1" | cut -d' ' -f2; }

# img_diff <png1> <png2> → % of pixels that changed (0-100, coarse)
img_diff() {
  python3 - "$1" "$2" <<'PY'
import sys
try:
    from PIL import Image, ImageChops
except ImportError:
    print("-1"); sys.exit(0)
try:
    a = Image.open(sys.argv[1]).convert("RGB").resize((64, 64))
    b = Image.open(sys.argv[2]).convert("RGB").resize((64, 64))
    diff = ImageChops.difference(a, b).convert("L")
    changed = sum(1 for p in diff.getdata() if p > 24)
    print(changed * 100 // (64 * 64))
except Exception:
    print("-1")
PY
}

# consecutive-screenshot stability. Loading screens ANIMATE → frames differ
# a lot; a settled screen (lobby, in-game after spawn) differs far less.
# wait_screen_stable <timeout_s> [stable_hits=3] [interval_s=3] [max_diff=2]
wait_screen_stable() {
  local timeout=${1:-300} need=${2:-3} interval=${3:-3} maxdiff=${4:-2}
  local t0=$(_now_s) hits=0 prev=""
  log "waiting for screen to settle (≤${timeout}s, need $need frames ≤${maxdiff}% diff)..."
  while :; do
    local cur; cur=$(screenshot stability) || cur=""
    if [ -n "$cur" ] && [ -n "$prev" ]; then
      local d; d=$(img_diff "$prev" "$cur")
      if [ "${d:-100}" -le "$maxdiff" ]; then
        hits=$((hits+1))
        [ $hits -ge "$need" ] && { ok "screen settled ($hits frames ≤${maxdiff}% diff)"; return 0; }
      else
        hits=0
      fi
    fi
    prev="$cur"
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { warn "screen never settled in ${timeout}s"; return 1; }
    sleep "$interval"
  done
}

# screen looks like a live rendered 3D scene (not black / not void):
# brightness in [8..250] and some color variance
shot_looks_ingame() {
  local f=$1 b c
  read -r b c <<< "$(img_stats "$f")"
  log "shot $(basename "$f"): brightness=${b} colorfulness=${c}"
  [ "${b:-0}" = "-1" ] && { warn "image analysis unavailable (no PIL)"; return 0; }
  [ "${b%.*}" -ge 8 ] && [ "${b%.*}" -le 250 ] && [ "${c%.*}" -ge 6 ]
}

# ─────────────────────────── uiautomator (browser etc.) ────────────────────
ui_dump() {  # → local xml path
  local remote=/sdcard/ft_ui_$(_ts).xml
  local local_f="$UI_DIR/ui_$(_ts).xml"
  shell_dev uiautomator dump "$remote" >/dev/null 2>&1 || return 1
  adb_dev pull "$remote" "$local_f" >/dev/null 2>&1 || return 1
  shell_dev rm -f "$remote" 2>/dev/null
  echo "$local_f"
}

# ui_find "Continue" → "x y" center of first node whose text/content-desc matches
ui_find() {
  local xml; xml=$(ui_dump) || return 1
  python3 - "$xml" "$1" <<'PY'
import sys, re
xml, needle = sys.argv[1], sys.argv[2]
best = None
for m in re.finditer(r'<node[^>]*/>|<node[^>]*>', xml):
    tag = m.group(0)
    tm = re.search(r'text="([^"]*)"', tag)
    dm = re.search(r'content-desc="([^"]*)"', tag)
    hay = ((tm.group(1) if tm else "") + " " + (dm.group(1) if dm else "")).lower()
    if needle.lower() in hay:
        bm = re.search(r'bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', tag)
        if bm:
            x = (int(bm.group(1)) + int(bm.group(3))) // 2
            y = (int(bm.group(2)) + int(bm.group(4))) // 2
            best = (x, y)
            break
if best:
    print(best[0], best[1])
PY
}

ui_tap_text() {  # <text> — find by text and tap it
  local xy; xy=$(ui_find "$1") || { err "ui: '$1' not found"; return 1; }
  log "ui: tapping '$1' at $xy"
  shell_dev input tap $xy
}

ui_wait_text() {  # <text> [timeout_s]
  local text=$1 timeout=${2:-60} t0=$(_now_s)
  log "ui: waiting for '$text' (≤${timeout}s)..."
  while :; do
    local xy; xy=$(ui_find "$text")
    [ -n "$xy" ] && { ok "ui: '$text' at $xy"; return 0; }
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { err "ui: '$text' never appeared"; return 1; }
    sleep 3
  done
}

# ─────────────────────────── browser + login flow ──────────────────────────
browser_open() {  # <url>
  log "browser: opening $1"
  shell_dev am start -a android.intent.action.VIEW -d "$1" >/dev/null 2>&1
}

# Voltronite login: open /login in the browser, enter the username, press
# Continue → page fires the com.epicgames.fortnite://authorize deep link →
# the game returns to the foreground and completes the oauth exchange.
login_via_browser() {  # [username]
  local user=${1:-$LOGIN_USER}
  hr "login flow (Voltronite via browser)"
  game_background           # game keeps running in the background
  sleep 2
  browser_open "$BACKEND_URL/login"
  sleep 4
  # Chrome first-run / welcome interstitials on fresh emulators
  ui_tap_text "Accept & continue" 2>/dev/null || true
  ui_tap_text "No thanks" 2>/dev/null || true
  sleep 2
  # wait for the Voltronite login page: username field + Continue button
  if ! ui_wait_text "Continue" 45; then
    screenshot login_page_missing
    err "login page did not load — is Voltronite reachable at $BACKEND_URL from the device?"
    return 1
  fi
  # focus the username field (the only <input>) and type
  local xy; xy=$(ui_find "Enter your username") || xy=$(ui_find "Voltronite")
  if [ -n "$xy" ]; then
    shell_dev input tap $xy
    sleep 1
    type_text "$user"
    sleep 1
  else
    warn "username field not found by label — tapping top-left of the card and typing anyway"
    tap_pct 50 38
    sleep 1
    type_text "$user"
  fi
  screenshot login_filled
  ui_tap_text "Continue" || return 1
  sleep 2
  # Chrome may show an "Open with Fortnite?" disambiguation for custom schemes
  ui_tap_text "Open" 2>/dev/null || true
  ui_tap_text "Just once" 2>/dev/null || true
  # the deep link brings the game back — NO relaunch, NO force-stop
  wait_game_foreground 60 || { screenshot login_no_return; return 1; }
  screenshot login_returned
  ok "login deep-link hop complete"
}

# ─────────────────────────── UE console (4-finger) ─────────────────────────
console_open() {
  log "console: opening via 4-finger tap..."
  four_finger_tap || return 1
  sleep 2
  screenshot console_opened
  ok "console should be visible (see shots/console_opened_*.png)"
}

# console_cmd "open Artemis_Terrain" — type into the console + Enter
console_cmd() {
  local cmd=$1
  log "console: $cmd"
  # make sure the console's edit box has focus (SConsole puts the input
  # line at the bottom of the window) — harmless if already focused
  tap_pct 50 85
  sleep 1
  type_text "$cmd"
  sleep 1
  key_enter
  sleep 2
  screenshot "console_$(echo "$cmd" | tr ' /' '__')"
}

console_close() {
  key_back
  sleep 1
  key_esc 2>/dev/null || true
}

# ─────────────────────────── game-flow milestones ──────────────────────────
# Erbium native logcat milestones (tag Erbium / Erbium-Owen)
ms_erbium_loaded()      { logcat_wait '=== Erbium Android bootstrap'      "${1:-180}" "liberbium.so loaded"; }
ms_engine_module()      { logcat_wait 'engine module: lib'                "${1:-900}" "engine module mapped"; }
ms_owen_installed()     { logcat_wait 'UE hooks installed'                "${1:-300}" "Owen redirect installed"; }
ms_release_probe()      { logcat_wait 'release probe OK'                  "${1:-300}" "bake verified vs live memory"; }
ms_gengine()            { logcat_wait 'GEngine = '                       "${1:-2700}" "GEngine singleton"; }
ms_wait_go()            { logcat_wait 'waiting for GO file'               "${1:-2700}" "GO-gate armed (waiting for login)"; }
ms_go_received()        { logcat_wait 'GO received|GO file already present' "${1:-120}" "GO signal"; }
ms_post_flip()          { logcat_wait 'POST-FLIP'                         "${1:-120}" "flag flip (process is now a server)"; }
ms_open_dispatched()    { logcat_wait 'dispatching console command'        "${1:-120}" "map open dispatched"; }

# how many Owen redirects have happened so far
redirect_count() { logcat_count 'Erbium-Owen.*redirect'; }

# wait_redirect_quiescence <idle_s> <timeout_s> — internal update is done
# when backend traffic stops for idle_s (cloudstorage sync finished).
wait_redirect_quiescence() {
  local idle=${1:-30} timeout=${2:-1800}
  local t0=$(_now_s) last_count=-1 last_change=$(_now_s)
  log "waiting for backend traffic to go quiet (idle ${idle}s, ≤${timeout}s)..."
  while :; do
    local c; c=$(redirect_count)
    if [ "$c" != "$last_count" ]; then
      last_count=$c; last_change=$(_now_s)
      log "backend traffic: $c redirected requests"
    fi
    if [ $(( $(_now_s) - last_change )) -ge "$idle" ]; then
      ok "backend traffic quiet for ${idle}s ($c requests total) — internal update done"
      return 0
    fi
    [ $(( $(_now_s) - t0 )) -ge "$timeout" ] && { warn "backend traffic never went quiet"; return 1; }
    sleep 5
  done
}

# touch the GO file — Erbium sees it and flips + opens the map
go_signal() {
  log "GO: touching $GO_FILE"
  shell_dev touch "$GO_FILE"
}

go_signal_clear() {
  log "GO: removing $GO_FILE (pre-test cleanup)"
  shell_dev rm -f "$GO_FILE" 2>/dev/null || true
}

# runtime backend URL config (read by liberbium at bootstrap): the URL the
# DEVICE uses to reach Voltronite — emulator host = http://10.0.2.2:3551,
# on-device Termux = http://127.0.0.1:3551
set_backend_url() {
  log "backend: $1 → /data/local/tmp/erbium.backend"
  shell_dev "echo '$1' > /data/local/tmp/erbium.backend"
  # keep the lib's browser login in sync with what the game will use
  BACKEND_URL=$1
}

# ─────────────────────────── verdict ───────────────────────────────────────
VERDICTS=()
verdict() { VERDICTS+=("$1"); if [ "$1" = PASS ]; then ok "VERDICT: $2"; else err "VERDICT: $2"; fi; }
verdicts_summary() {
  hr "verdict"
  local pass=1
  for v in "${VERDICTS[@]}"; do [ "$v" = FAIL ] && pass=0; done
  if [ ${#VERDICTS[@]} -eq 0 ]; then pass=0; fi
  if [ "$pass" = 1 ]; then
    ok "ALL CHECKS PASSED"
    echo "OVERALL: PASS" >> "$ARTIFACTS/VERDICT.txt"
  else
    err "SOME CHECKS FAILED — see $ARTIFACTS"
    echo "OVERALL: FAIL" >> "$ARTIFACTS/VERDICT.txt"
  fi
  [ -n "${GITHUB_STEP_SUMMARY:-}" ] && {
    {
      echo "## Game-flow test"
      echo '```'
      cat "$ARTIFACTS/VERDICT.txt" 2>/dev/null
      echo '```'
    } >> "$GITHUB_STEP_SUMMARY"
  }
  [ "$pass" = 1 ]
}
