#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fc3_h3_run.py - one-command launcher for the H3 live test against a running Far Cry 3.

Wraps scripts/fc3_h3_trace.js in a Python driver so the whole session is captured
to a log file (frida's own REPL + a pipe to a file is fragile because prompt-toolkit
does not like a redirected stdout). This driver:
  * finds the running game process (farcry3.exe / farcry3_d3d11.exe) and attaches,
  * loads fc3_h3_trace.js,
  * mirrors every script console.log/send/error to the console AND to h3_live.log,
  * offers a small command loop (dump / mode / switch dry|go / pop dry|go) that maps
    to the script's rpc.exports, logging each result.

The dangerous RPC levers (switchCoop/populate live calls) require an explicit "go".
Run OFFLINE only; working copy only. See docs/h3-live-test.md (+ .ru.md).

Usage:
  python scripts/fc3_h3_run.py            # auto-detect the FC3 game process
  python scripts/fc3_h3_run.py --pid 1234 # attach to a specific pid
"""
import argparse
import datetime
import json
import os
import sys

try:
    import frida
except ImportError:
    sys.stderr.write("frida is not installed. Run: python -m pip install frida-tools\n")
    sys.exit(1)

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT_JS = os.path.join(HERE, "fc3_h3_trace.js")
LOG_DIR = r"H:\FarCryCoop\scratch"
LOG_PATH = os.path.join(LOG_DIR, "h3_live.log")
# Process image names the game can run as (the DLL is FC3.dll, but the EXE is farcry3*).
GAME_PROCS = ("farcry3", "farcry3_d3d11")


def ts():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


class Logger:
    def __init__(self, path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        backup = None
        if os.path.exists(path):
            stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            backup = path + "." + stamp + ".bak"
            os.replace(path, backup)
        self.f = open(path, "a", encoding="utf-8", newline="\n")
        self.echo("=== fc3_h3_run session %s ===" % datetime.datetime.now().isoformat(timespec="seconds"))
        self.echo("log file: %s" % path)
        if backup:
            self.echo("(previous log saved to %s)" % backup)

    def echo(self, line):
        # print tolerates the cp1251 console; file is utf-8.
        try:
            print(line, flush=True)
        except UnicodeEncodeError:
            sys.stdout.buffer.write((line + "\n").encode("utf-8", "replace"))
            sys.stdout.flush()
        self.f.write(line + "\n")
        self.f.flush()

    def close(self):
        try:
            self.f.close()
        except Exception:
            pass


def find_game_pid(device, log):
    procs = device.enumerate_processes()
    hits = [p for p in procs if p.name and p.name.lower().rsplit(".", 1)[0] in GAME_PROCS]
    if not hits:
        log.echo("No FC3 game process found (looked for: %s)." % ", ".join(GAME_PROCS))
        log.echo("Launch H:\\FarCryCoop\\TestFC3\\bin\\farcry3.exe OFFLINE to the main menu, wait until you")
        log.echo("ARE at the main menu, then re-run. If the game IS running but still not found:")
        log.echo("  - it may be launched as Administrator: a non-elevated Python/Frida cannot see or")
        log.echo("    attach an elevated process. Launch the game WITHOUT 'Run as administrator', or run")
        log.echo("    this launcher from an elevated shell.")
        log.echo("  - to see every process Frida can actually see, run: fc3_h3_run.bat --list")
        log.echo("Or pass the pid explicitly: python scripts/fc3_h3_run.py --pid <pid>")
        return None
    for p in hits:
        log.echo("found game process: pid=%d name=%s" % (p.pid, p.name))
    if len(hits) > 1:
        log.echo("Multiple candidates; attaching to the first. Use --pid to choose another.")
    return hits[0].pid


def to_snake(name):
    return "".join("_" + c.lower() if c.isupper() else c for c in name)


def main():
    ap = argparse.ArgumentParser(description="H3 live-test launcher")
    ap.add_argument("--pid", type=int, default=None, help="attach to this pid instead of auto-detect")
    ap.add_argument("--list", action="store_true", help="list processes Frida can see (diagnose attach) and exit")
    ap.add_argument("--grep", default=None, help="with --list, only names containing this substring")
    args = ap.parse_args()

    log = Logger(LOG_PATH)
    try:
        device = frida.get_local_device()
        if args.list:
            ps = sorted(device.enumerate_processes(), key=lambda p: (p.name or "").lower())
            log.echo("Frida sees %d processes:" % len(ps))
            for p in ps:
                if args.grep and args.grep.lower() not in (p.name or "").lower():
                    continue
                log.echo("  pid=%d  %s" % (p.pid, p.name))
            return 0
        pid = args.pid or find_game_pid(device, log)
        if pid is None:
            return 2
        log.echo("attaching to pid %d ..." % pid)
        session = device.attach(pid)

        with open(SCRIPT_JS, "r", encoding="utf-8") as fh:
            code = fh.read()
        log.echo("loading %s (%d bytes)" % (os.path.basename(SCRIPT_JS), len(code)))

        def on_message(message, data):
            mtype = message.get("type")
            if mtype == "log":
                lvl = message.get("level", "info")
                log.echo("[script:%s] %s" % (lvl, message.get("payload", "")))
            elif mtype == "send":
                log.echo("[send] %s" % json.dumps(message.get("payload"), ensure_ascii=False))
            elif mtype == "error":
                log.echo("[script-error] %s" % message.get("description", message))
                if message.get("stack"):
                    log.echo("  " + message["stack"].replace("\n", "\n  "))
            else:
                log.echo("[msg] %s" % json.dumps(message, ensure_ascii=False))

        script = session.create_script(code)
        script.on("message", on_message)
        script.load()
        exports = script.exports_sync

        def call(jsname, *cargs):
            snake = to_snake(jsname)
            for n in (snake, jsname):
                if hasattr(exports, n):
                    return getattr(exports, n)(*cargs)
            raise AttributeError("rpc export %s (also tried %s) not found" % (jsname, snake))

        HELP = (
            "commands:\n"
            "  dump                read live playlist-service container  (dumpService)\n"
            "  mode                last mode-manager this + [this+0x4c]  (lastMode)\n"
            "  switch dry          H3a dry run (no live call)            (switchCoop true)\n"
            "  switch go           H3a LIVE: write [this+0x4c]+call vtable[+0x5c] (may crash)\n"
            "  pop dry             H3b dry run (show svcInit addr)       (populate true)\n"
            "  pop go              H3b LIVE: call FUN_1127a0f0(service)  (may crash)\n"
            "  modeid <this> <id>  DANGEROUS poke: setModeId(this_hex, id_hex)\n"
            "  help | quit"
        )
        log.echo(HELP)
        log.echo("First: do `dump`, enter+leave a menu in-game, then `mode` (need a non-null thisPtr),")
        log.echo("then `switch dry`, then `switch go`. Watch the game window AND this log on every 'go'.")

        while True:
            try:
                line = input("fc3> ").strip()
            except EOFError:
                break
            if not line:
                continue
            log.echo("fc3> " + line)
            parts = line.split()
            cmd, rest = parts[0].lower(), parts[1:]
            try:
                if cmd in ("dump", "dumpservice"):
                    log.echo(json.dumps(call("dumpService"), ensure_ascii=False))
                elif cmd in ("mode", "lastmode"):
                    r = call("lastMode")
                    log.echo(json.dumps(r, ensure_ascii=False))
                elif cmd == "switch":
                    if not rest or rest[0].lower() not in ("dry", "go"):
                        log.echo("use: switch dry | switch go")
                        continue
                    dry = rest[0].lower() == "dry"
                    if not dry:
                        log.echo("LIVE call: writing [this+0x4c]=CO-OP and calling modeManager vtable[+0x5c].")
                        log.echo("Watch the game window. A crash here is still useful data.")
                    log.echo(json.dumps(call("switchCoop", dry), ensure_ascii=False))
                elif cmd == "pop":
                    if not rest or rest[0].lower() not in ("dry", "go"):
                        log.echo("use: pop dry | pop go")
                        continue
                    dry = rest[0].lower() == "dry"
                    if not dry:
                        log.echo("LIVE call: FUN_1127a0f0(service) populates the co-op container (may crash).")
                    log.echo(json.dumps(call("populate", dry), ensure_ascii=False))
                elif cmd == "modeid":
                    if len(rest) != 2:
                        log.echo("use: modeid <this_hex> <id_hex>")
                        continue
                    log.echo(json.dumps(call("setModeId", rest[0], int(rest[1], 16)), ensure_ascii=False))
                elif cmd in ("help", "?", "h"):
                    log.echo(HELP)
                elif cmd in ("quit", "exit", "q"):
                    break
                else:
                    log.echo("unknown command: %s (type 'help')" % cmd)
            except Exception as exc:  # keep the loop alive; log everything
                log.echo("ERROR: %r" % (exc,))

        log.echo("detaching")
        try:
            script.unload()
        except Exception:
            pass
        try:
            session.detach()
        except Exception:
            pass
    finally:
        log.echo("=== session end %s ===" % datetime.datetime.now().isoformat(timespec="seconds"))
        log.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
