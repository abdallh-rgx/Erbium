#!/usr/bin/env python3
"""panel.py — phone-friendly control panel for the Android emulator.

Served on the runner (bound to all interfaces; reach it through Tailscale).
One page: live screenshot + big buttons for the actions a phone can't do
(multitouch console, deep-link authorize, adb commands).
"""
import html
import os
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PORT = int(os.environ.get("PANEL_PORT", "8081"))
LAST_SHOT = "/tmp/panel_last.png"
SHOT_LOCK = threading.Lock()


def sh(cmd, timeout=20):
    """Run a shell command, return (rc, output)."""
    try:
        p = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
        return p.returncode, (p.stdout or "") + (p.stderr or "")
    except subprocess.TimeoutExpired:
        return 124, "timeout"
    except Exception as e:
        return 1, str(e)


def adb(cmd, timeout=20):
    return sh(f"adb {cmd}", timeout)


def screenshot():
    with SHOT_LOCK:
        adb("shell screencap -p /sdcard/panel_shot.png", 15)
        adb(f"pull /sdcard/panel_shot.png {LAST_SHOT}", 15)
        adb("shell rm -f /sdcard/panel_shot.png", 10)
        try:
            return open(LAST_SHOT, "rb").read()
        except Exception:
            return b""


def four_finger():
    """4-finger tap via the emulator console (opens the UE console)."""
    cmds = [
        # press + 4 slots down
        "adb emu event send EV_KEY:BTN_TOUCH:1 "
        "EV_ABS:ABS_MT_SLOT:0 EV_ABS:ABS_MT_TRACKING_ID:400 "
        "EV_ABS:ABS_MT_POSITION_X:216 EV_ABS:ABS_MT_POSITION_Y:798 "
        "EV_ABS:ABS_MT_PRESSURE:50 EV_ABS:ABS_MT_TOUCH_MAJOR:10 "
        "EV_ABS:ABS_MT_SLOT:1 EV_ABS:ABS_MT_TRACKING_ID:401 "
        "EV_ABS:ABS_MT_POSITION_X:864 EV_ABS:ABS_MT_POSITION_Y:798 "
        "EV_ABS:ABS_MT_SLOT:2 EV_ABS:ABS_MT_TRACKING_ID:402 "
        "EV_ABS:ABS_MT_POSITION_X:216 EV_ABS:ABS_MT_POSITION_Y:1482 "
        "EV_ABS:ABS_MT_SLOT:3 EV_ABS:ABS_MT_TRACKING_ID:403 "
        "EV_ABS:ABS_MT_POSITION_X:864 EV_ABS:ABS_MT_POSITION_Y:1482 "
        "EV_SYN:SYN_REPORT:0",
        "sleep 0.5",
        # release all
        "adb emu event send EV_KEY:BTN_TOUCH:0 "
        "EV_ABS:ABS_MT_SLOT:0 EV_ABS:ABS_MT_TRACKING_ID:4294967295 "
        "EV_ABS:ABS_MT_SLOT:1 EV_ABS:ABS_MT_TRACKING_ID:4294967295 "
        "EV_ABS:ABS_MT_SLOT:2 EV_ABS:ABS_MT_TRACKING_ID:4294967295 "
        "EV_ABS:ABS_MT_SLOT:3 EV_ABS:ABS_MT_TRACKING_ID:4294967295 "
        "EV_SYN:SYN_REPORT:0",
    ]
    out = []
    for c in cmds:
        rc, o = sh(c)
        out.append(o.strip())
    return "\n".join(out) or "sent"


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _send(self, body, ctype="text/html; charset=utf-8", code=200):
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        qs = {}
        if "?" in self.path:
            for kv in self.path.split("?", 1)[1].split("&"):
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    qs[k] = v

        if path == "/" or path == "/index.html":
            self._send(INDEX)
        elif path == "/shot.png":
            self._send(screenshot(), "image/png")
        elif path == "/api":
            action = qs.get("action", "")
            out = self.api(action, qs)
            self._send(f"<pre>{html.escape(out)}</pre>"
                       f'<p><a href="/">← back</a></p>')
        else:
            self._send("not found", code=404)

    def api(self, action, qs):
        if action == "console":
            return four_finger()
        if action == "tap":
            return adb(f"shell input tap {qs.get('x','540')} {qs.get('y','1140')}")[1] or "tapped"
        if action == "swipe":
            return adb(f"shell input swipe {qs.get('x1','540')} {qs.get('y1','1400')} "
                       f"{qs.get('x2','540')} {qs.get('y2','600')} {qs.get('ms','300')}")[1] or "swiped"
        if action == "key":
            return adb(f"shell input keyevent {qs.get('k','4')}")[1] or "key sent"
        if action == "text":
            return adb(f"shell input text '{qs.get('t','')}'")[1] or "typed"
        if action == "authorize":
            # OGFN AUTHORIZE LOGIN equivalent: fire the deep link with
            # LawinServer's exchange code (works while the game is in the
            # background of the login screen)
            scheme = qs.get("scheme", "com.epicgames.fortnit1840")
            return adb(f"shell am start -a android.intent.action.VIEW -d "
                       f"'{scheme}://authorize/?code=lawinsexchangecodelol'")[1] or "authorize fired"
        if action == "go":
            return adb("shell touch /data/local/tmp/erbium.go")[1] or "GO file touched"
        if action == "shell":
            cmd = qs.get("cmd", "id")
            return f"$ {cmd}\n" + sh(cmd)[1]
        if action == "adb":
            cmd = qs.get("cmd", "devices")
            return f"$ adb {cmd}\n" + adb(cmd)[1]
        if action == "logcat":
            n = qs.get("n", "60")
            return adb(f"logcat -d -t {n}")[1]
        if action == "netstats":
            return sh("adb shell 'cat /proc/net/udp /proc/net/tcp 2>/dev/null | grep -cE \"1E61|1F90\"'")[1]
        if action == "state":
            game = qs.get("pkg", "com.epicgames.fortnit1840")
            rc, pid = adb(f"shell pidof {game}")
            rc2, focus = adb("shell dumpsys window 2>/dev/null | grep -m1 mCurrentFocus")
            udp, _ = sh("adb shell 'cat /proc/net/udp /proc/net/udp6 2>/dev/null | grep -c 1E61'")
            return (f"game pid: {pid.strip() or 'DEAD'}\n"
                    f"{focus.strip()}\n"
                    f"UDP :7777 listeners: {udp.strip()}")
        return "unknown action"


INDEX = """<!doctype html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Erbium Remote</title>
<style>
body{font-family:system-ui;margin:8px;background:#111;color:#eee}
img{max-width:100%;border:1px solid #444;border-radius:8px}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin:10px 0}
button{padding:14px;font-size:16px;border-radius:10px;border:0;background:#2e7d32;color:#fff}
button.alt{background:#455a64}
button.warn{background:#b71c1c}
input{width:100%;padding:10px;font-size:15px;border-radius:8px;border:1px solid #555;background:#222;color:#eee;box-sizing:border-box}
h3{margin:12px 0 4px}
</style></head><body>
<h3>📱 Erbium Remote — emulator control</h3>
<img id="shot" src="/shot.png" onclick="tapAt(event)" onerror="this.src='/shot.png'">
<div class="grid">
  <button onclick="act('console')">🎮 Console (4-touch)</button>
  <button class="alt" onclick="act('go')">🚀 GO (Erbium)</button>
  <button class="alt" onclick="act('authorize')">🔑 AUTHORIZE LOGIN</button>
  <button class="alt" onclick="act('key&k=4')">↩ Back</button>
  <button class="alt" onclick="act('key&k=3')">🏠 Home</button>
  <button class="alt" onclick="act('key&k=66')">⏎ Enter</button>
  <button class="alt" onclick="refresh()">🔄 Screenshot</button>
  <button class="warn" onclick="act('state')">📊 State</button>
</div>
<h3>⌨️ Type text into the device</h3>
<input id="txt" placeholder="type then press Enter" onkeydown="if(event.key==='Enter'){byId('textbtn').click()}">
<div class="grid">
  <button id="textbtn" class="alt" onclick="typeText()">Send text</button>
  <button class="alt" onclick="act('logcat&n=80')">📋 Logcat (80)</button>
</div>
<h3>🛠 ADB / Shell</h3>
<input id="cmd" placeholder="adb command (e.g. shell pm list packages)">
<div class="grid">
  <button class="alt" onclick="adbcmd()">Run adb</button>
  <button class="warn" onclick="shellcmd()">Run shell</button>
</div>
<pre id="out" style="background:#000;color:#0f0;padding:8px;border-radius:8px;white-space:pre-wrap;font-size:12px;max-height:40vh;overflow:auto"></pre>
<script>
const byId = id => document.getElementById(id);
function refresh(){ byId('shot').src = '/shot.png?' + Date.now(); }
setInterval(refresh, 5000);
function act(a){
  byId('out').textContent = '...';
  fetch('/api?action=' + a).then(r=>r.text()).then(t=>{byId('out').innerHTML = t; refresh();});
}
function tapAt(ev){
  const img = byId('shot');
  const r = img.getBoundingClientRect();
  const x = Math.round((ev.clientX - r.left) / r.width * 1080);
  const y = Math.round((ev.clientY - r.top) / r.height * 2280);
  act('tap&x=' + x + '&y=' + y);
}
function typeText(){
  const t = encodeURIComponent(byId('txt').value);
  act('text&t=' + t);
}
function adbcmd(){ act('adb&cmd=' + encodeURIComponent(byId('cmd').value)); }
function shellcmd(){ act('shell&cmd=' + encodeURIComponent(byId('cmd').value)); }
</script>
</body></html>"""


if __name__ == "__main__":
    print(f"[panel] listening on :{PORT}")
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
