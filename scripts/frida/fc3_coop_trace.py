#!/usr/bin/env python
"""fc3_coop_trace.py - attach Frida to the running Far Cry 3 (farcry3_d3d11.exe) and run fc3_coop_trace.js.

Procedure: launch FC3 via Ubisoft Connect, reach the main menu, THEN run this, THEN navigate to co-op.
Reads nothing from drive C:; writes the trace to local/scratch/frida_coop_<ts>.log.
"""
import sys, os, time
import frida

PROC = "farcry3_d3d11.exe"
HERE = os.path.dirname(os.path.abspath(__file__))
JS = os.path.join(HERE, "fc3_coop_trace.js")
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
OUTDIR = os.path.join(REPO, "local", "scratch")
os.makedirs(OUTDIR, exist_ok=True)
OUT = os.path.join(OUTDIR, "frida_coop_%s.log" % time.strftime("%Y%m%d_%H%M%S"))

def wf(line):
    try:
        with open(OUT, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception as e:
        print("[write-error] %s" % e, flush=True)

def on_message(message, data):
    if message.get("type") == "send":
        line = str(message.get("payload"))
    elif message.get("type") == "error":
        line = "[frida-error] " + message.get("description", str(message))
    else:
        line = str(message)
    print(line, flush=True); wf(line)

def on_log(level, text):
    line = "[js-log %s] %s" % (level, text)
    print(line, flush=True); wf(line)

def main():
    devs = frida.enumerate_devices()
    local = next((d for d in devs if d.type == "local"), None)
    if not local:
        print("no local frida device"); return 2
    sessions = [p for p in local.enumerate_processes() if p.name.lower() == PROC.lower()]
    if not sessions:
        print("process %s not running. Launch FC3 via Ubisoft Connect, reach main menu, then rerun this." % PROC)
        return 3
    pid = sessions[0].pid
    print("attaching to %s pid=%d ; writing %s" % (PROC, pid, OUT))
    wf("ATTACH-TRY pid=%d" % pid)
    session = None
    last = None
    for attempt in range(1, 9):
        try:
            session = frida.attach(pid)
            break
        except Exception as e:
            last = e
            print("attach attempt %d failed: %s (retry in 2.5s; make sure the game is idle at the main menu)" % (attempt, e), flush=True)
            wf("ATTACH-FAIL %d %s" % (attempt, e))
            time.sleep(2.5)
    if session is None:
        print("could not attach after retries: %s\nFully quit the game (and any leftover python/frida), relaunch, get to the main menu, and rerun." % last)
        wf("ATTACH-ABORT %s" % last)
        return 4
    session = session  # attached above
    script = session.create_script(open(JS, encoding="utf-8").read())
    script.on("message", on_message)
    try:
        script.on("log", on_log)
    except Exception:
        pass
    wf("ATTACHED pid=%d %s" % (pid, PROC))
    script.load()
    print("hooked. In-game: main menu -> co-op -> connect. Ctrl+C to stop.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    try: session.detach()
    except Exception: pass
    return 0

if __name__ == "__main__":
    sys.exit(main())
