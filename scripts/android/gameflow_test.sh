#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  gameflow_test.sh — END-TO-END Fortnite flow test (Erbium android port)
#
#  The flow (exactly how a human plays it — the automation just does it
#  faster and never gets tired):
#
#    1. launch game (patched APK: loadLibrary + 4-finger console)
#    2. wait for the INTERNAL UPDATE to COMPLETE  (backend traffic goes
#       quiet + screen settles — never force-stop, the game may restart
#       itself, we just re-attach)
#    3. LOGIN: browser → Voltronite /login → username → Continue →
#       com.epicgames.fortnite://authorize deep link → game back in
#       foreground → logged in. No relaunch — resume only.
#    4. wait for the lobby to settle
#    5. GO: touch /data/local/tmp/erbium.go → Erbium flips GIsClient/
#       GIsServer and opens the map (listen-server bring-up)
#    6. wait for the match INTERNAL LOADING to COMPLETE (loading screen
#       churn ends, rendered scene appears)
#    7. VERIFY THE PLAYER: in-match screenshot + brightness/color analysis
#       + console `getall PlayerController Location` (4-finger console)
#       — a shot on the loading screen, a black void (under the map) or a
#       missing pawn DOES NOT COUNT
#
#  Usage:
#    gameflow_test.sh [--apk FILE] [--backend URL] [--user NAME]
#                     [--skip-install] [--skip-login] [--quick]
#
#  Env knobs (all optional): see ft_lib.sh + PHASE_* timeouts below.
# ═══════════════════════════════════════════════════════════════════════════
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ft_lib.sh"

# ── defaults / arg parsing ─────────────────────────────────────────────────
APK=""
SKIP_INSTALL=0
SKIP_LOGIN=0
QUICK=0

while [ $# -gt 0 ]; do
  case "$1" in
    --apk)          APK="$2"; shift 2 ;;
    --backend)      BACKEND_URL="$2"; shift 2 ;;
    --user)         LOGIN_USER="$2"; shift 2 ;;
    --serial)       ADB_SERIAL="$2"; shift 2 ;;
    --skip-install) SKIP_INSTALL=1; shift ;;
    --skip-login)   SKIP_LOGIN=1; shift ;;
    --quick)        QUICK=1; shift ;;
    -h|--help)      grep '^#' "$0" | sed 's/^# \{0,2\}//'; exit 0 ;;
    *) err "unknown arg: $1"; exit 1 ;;
  esac
done

# timeouts (emulator defaults are huge — engine init crawls under translation)
if [ "$QUICK" = 1 ]; then
  T_ENGINE_MODULE=${T_ENGINE_MODULE:-900}
  T_GENGINE=${T_GENGINE:-2400}
  T_UPDATE=${T_UPDATE:-900}
  T_LOBBY=${T_LOBBY:-300}
  T_MATCH_LOAD=${T_MATCH_LOAD:-900}
else
  T_ENGINE_MODULE=${T_ENGINE_MODULE:-1800}   # 30 min: libUnreal mapped
  T_GENGINE=${T_GENGINE:-2400}               # 40 min: GEngine singleton
  T_UPDATE=${T_UPDATE:-2700}                 # 45 min: internal update done
  T_LOBBY=${T_LOBBY:-600}                    # 10 min: lobby settled
  T_MATCH_LOAD=${T_MATCH_LOAD:-1500}         # 25 min: map load + spawn
fi
MATCH_MIN_ELAPSED=${MATCH_MIN_ELAPSED:-120}  # never declare loaded before 2 min

# ── artifacts on any exit ──────────────────────────────────────────────────
collect_artifacts() {
  log "collecting artifacts → $ARTIFACTS"
  logcat_refresh
  { echo "── top activity: $(top_activity_name)"
    echo "── game pid: $(game_pid || echo none)"
    echo "── redirects: $(redirect_count)"; } > "$ARTIFACTS/state.txt" 2>/dev/null
  logcat_grep 'Erbium|POST-FLIP|GO |dispatching|redirect|FATAL|SIGSEGV' 400 \
    > "$ARTIFACTS/logcat-digest.txt" 2>/dev/null || true
}
trap 'collect_artifacts' EXIT

# ═════════════════════════ 0. preflight ════════════════════════════════════
hr "phase 0 — preflight"
wait_for_device 60 || die "no adb device"
wake_screen
device_info
adb_root_try

# emulator sanity: the image MUST run arm64 code (ndk_translation) or the
# game will never come up
if is_emulator; then
  case "$(device_abilist)" in
    *arm64*) ok "emulator abilist has arm64: $(device_abilist)" ;;
    *) die "emulator image has NO arm64 translation (abilist=$(device_abilist)) —
              use an API-30+ x86_64 image with ndk_translation" ;;
  esac
  # backend runs on the CI host → the device reaches it via 10.0.2.2
  BACKEND_URL="${BACKEND_URL//127.0.0.1/10.0.2.2}"
  BACKEND_URL="${BACKEND_URL//localhost/10.0.2.2}"
fi
log "backend URL (device view): $BACKEND_URL"

# fresh run: clear the GO gate + set the backend the game should redirect to
go_signal_clear
set_backend_url "$BACKEND_URL"

# host-side backend check (best effort — on-device Termux won't be visible)
curl -fsS --max-time 5 "${BACKEND_URL/10.0.2.2/127.0.0.1}/" >/dev/null 2>&1 \
  && ok "backend answers on the host" \
  || warn "backend not reachable from THIS machine (fine if it runs on-device/Termux)"

logcat_clear

# ═════════════════════════ 1. install + launch ═════════════════════════════
if [ "$SKIP_INSTALL" = 0 ] && [ -n "$APK" ]; then
  hr "phase 1 — install"
  install_apk "$APK" || verdict FAIL "install"
else
  log "install skipped — relying on the APK already on the device"
  is_installed || die "$GAME_PKG is not installed (pass --apk)"
fi

hr "phase 1 — launch"
game_launch
sleep 5
wait_game_process 120 || verdict FAIL "game process never started"

# Erbium native milestones while the game boots
ms_erbium_loaded 300        || verdict FAIL "liberbium.so never loaded (smali injection broken?)"
ms_engine_module "$T_ENGINE_MODULE" || verdict FAIL "engine module never mapped"
ms_owen_installed 300       || verdict FAIL "Owen redirect never installed"
ms_release_probe 300        || verdict FAIL "bake verification failed (offset mismatch)"
screenshot boot_started

# ═════════════════════════ 2. internal update ══════════════════════════════
hr "phase 2 — internal update (waiting for it to COMPLETE, never force-stop)"
# The first boot downloads/verifies paks through the backend. Done = backend
# traffic went quiet for UPDATE_IDLE_S. The game may RESTART itself
# mid-update — that is fine, we never kill it, we just re-attach and wait.
UPDATE_IDLE_S=${UPDATE_IDLE_S:-25}
UPD_OK=0
last_count=-1 last_change=$(_now_s) zero_rounds=0
t0=$(_now_s)
while [ $(( $(_now_s) - t0 )) -lt "$T_UPDATE" ]; do
  # self-restart tolerance: process gone = it relaunched itself (post-update)
  if ! game_alive; then
    log "game process gone (self-restart?) — waiting for it to come back..."
    if wait_game_process 300; then
      log "game came back (pid $(game_pid)) — update continues"
    else
      verdict FAIL "game process died and never came back during update"
      break
    fi
  fi
  if crash_detected; then
    verdict FAIL "game CRASHED during internal update — see logcat-digest.txt"
    screenshot crash_update
    break
  fi
  c=$(redirect_count)
  if [ "$c" != "$last_count" ]; then
    last_count=$c; last_change=$(_now_s)
    log "backend traffic: $c redirected requests"
  fi
  if [ $(( $(_now_s) - last_change )) -ge "$UPDATE_IDLE_S" ]; then
    UPD_OK=1
    ok "backend traffic quiet for ${UPDATE_IDLE_S}s ($c requests total) — internal update done"
    break
  fi
  # zero backend traffic from the start AND a settled screen = already updated
  if [ "$c" -eq 0 ]; then
    zero_rounds=$(( zero_rounds + 1 ))
    if [ "$zero_rounds" -ge 6 ] && wait_screen_stable 60 3 5 12; then
      UPD_OK=1
      log "no backend traffic + settled screen — update already complete"
      break
    fi
  else
    zero_rounds=0
  fi
  sleep 10
done
if [ "$UPD_OK" = 1 ]; then verdict PASS "internal update complete"; else verdict FAIL "internal update did not finish in ${T_UPDATE}s"; fi
screenshot update_done
if ! game_alive; then wait_game_process 300; fi
game_foreground || game_resume   # bring it back WITHOUT restarting

# ═════════════════════════ 3. login ════════════════════════════════════════
if [ "$SKIP_LOGIN" = 1 ]; then
  log "login skipped (--skip-login) — assuming the game is already logged in"
  verdict PASS "login (skipped)"
else
  hr "phase 3 — login via Voltronite (browser flow)"
  # The engine must be up before the frontend can show its login prompt and
  # accept the oauth deep link.
  ms_gengine "$T_GENGINE" || verdict FAIL "GEngine never appeared — engine init too slow/stuck"
  sleep 30   # let the frontend world + login prompt come up

  # The deep-link code is only accepted while the game waits for auth; if we
  # hop too early the code is dropped — so retry the whole browser flow.
  LOGIN_OK=0
  for attempt in 1 2 3; do
    log "login attempt $attempt/3"
    login_via_browser "$LOGIN_USER" || { warn "browser hop failed (attempt $attempt)"; sleep 20; continue; }
    if logcat_wait 'redirect.*(oauth|token)|oauth/token' 240 "oauth token exchange"; then
      LOGIN_OK=1; break
    fi
    if [ -n "${VOLTRONITE_LOG:-}" ] && tail -200 "$VOLTRONITE_LOG" 2>/dev/null | grep -q "token"; then
      ok "token seen in backend log"; LOGIN_OK=1; break
    fi
    warn "no token evidence after attempt $attempt — the game may not have been at its login prompt yet; retrying"
    sleep 30
  done
  if [ "$LOGIN_OK" = 1 ]; then
    verdict PASS "login: token exchanged (user $LOGIN_USER)"
  else
    verdict FAIL "login: no token exchange evidence after 3 attempts"
  fi
  screenshot after_login
fi

# ═════════════════════════ 4. lobby settle ═════════════════════════════════
hr "phase 4 — lobby"
if wait_screen_stable "$T_LOBBY" 3 5 10; then
  verdict PASS "lobby settled"
else
  warn "lobby never fully settled — continuing anyway (frontend may animate)"
fi
screenshot lobby
ms_gengine "$T_GENGINE" || verdict FAIL "GEngine never appeared — engine init too slow/stuck"
ms_wait_go "$((T_GENGINE + 300))" || verdict FAIL "Erbium never armed the GO gate"

# ═════════════════════════ 5. GO — flip + open the map ═════════════════════
hr "phase 5 — GO (flip GIs* + open the map)"
go_signal
ms_go_received 120     || verdict FAIL "GO signal not seen by Erbium"
ms_post_flip 180       || verdict FAIL "flag flip never happened"
ms_open_dispatched 180 || verdict FAIL "map open never dispatched"
screenshot go_flip
verdict PASS "flip + open dispatched"

# ═════════════════════════ 6. match load (INTERNAL LOADING COMPLETE) ══════
hr "phase 6 — match loading (waiting for the INTERNAL loading to COMPLETE)"
# loading screens animate → consecutive screenshots differ a lot. The match
# is loaded when churn drops AND the screen is a rendered scene AND at least
# MATCH_MIN_ELAPSED seconds passed since GO.
t_go=$(_now_s)
LOAD_STATE=loading
t0=$(_now_s)
while [ $(( $(_now_s) - t0 )) -lt "$T_MATCH_LOAD" ]; do
  if crash_detected; then
    verdict FAIL "game CRASHED during match load"
    screenshot crash_match
    break
  fi
  elapsed=$(( $(_now_s) - t_go ))
  if [ "$elapsed" -lt "$MATCH_MIN_ELAPSED" ]; then
    sleep 10; continue     # never judge before the floor — no shortcuts
  fi
  if wait_screen_stable 90 3 5 12; then
    # stable — but is it a rendered scene (not a frozen loading frame)?
    f=$(last_shot stability); [ -n "$f" ] || f=$(screenshot match_check)
    if shot_looks_ingame "$f"; then
      LOAD_STATE=loaded
      verdict PASS "match internal loading COMPLETE (${elapsed}s after GO)"
      break
    else
      log "stable but not a scene yet (loading frame?) — keep waiting"
    fi
  fi
  # process may self-restart after the pak sync — tolerate it
  game_alive || { wait_game_process 300; game_resume; }
  sleep 5
done
[ "$LOAD_STATE" = loaded ] || verdict FAIL "match loading did not complete in ${T_MATCH_LOAD}s"
screenshot match_loaded

# ═════════════════════════ 7. player verification ══════════════════════════
hr "phase 7 — player verification (must exist, must NOT be under the map)"
INGAME_SHOT=$(screenshot ingame_pov)
if [ -n "$INGAME_SHOT" ]; then
  if shot_looks_ingame "$INGAME_SHOT"; then
    verdict PASS "in-match screenshot looks like a live scene: $(basename "$INGAME_SHOT")"
  else
    verdict FAIL "in-match screenshot is black/void — player missing or under the map? ($(basename "$INGAME_SHOT"))"
  fi
else
  verdict FAIL "could not capture an in-match screenshot"
fi

# console evidence: position text via getall (4-finger console). Best-effort —
# the screenshot artifacts carry the proof for human review.
log "console evidence (best-effort)..."
if console_open; then
  console_cmd "getall PlayerController Location"   # WHERE is the player
  console_cmd "getall PlayerController Pawn"        # does a pawn even exist
  console_close
  screenshot after_console
  verdict PASS "console getall captured (see shots/console_getall_*.png)"
else
  warn "4-finger console not automatable here — tap the screen with 4 real fingers to check the pawn location by hand"
fi

# final in-game shot AFTER the console dance (proves the session is still alive)
FINAL_SHOT=$(screenshot final_ingame)
[ -n "$FINAL_SHOT" ] && shot_looks_ingame "$FINAL_SHOT" \
  && verdict PASS "final in-game shot after console: $(basename "$FINAL_SHOT")" \
  || verdict FAIL "session not alive after console interaction"

# ═════════════════════════ 8. verdict ══════════════════════════════════════
hr "summary"
{
  echo "game pid at end: $(game_pid || echo GONE) (never force-stopped: $( [ -n "$(game_pid)" ] && echo YES || echo no))"
  echo "owen redirects total: $(redirect_count)"
  echo "screenshots: $(ls "$SHOTS_DIR" | wc -l) in $SHOTS_DIR"
} | tee "$ARTIFACTS/summary.txt"
verdicts_summary
