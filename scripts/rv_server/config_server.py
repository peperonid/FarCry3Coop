#!/usr/bin/env python3
"""config_server.py - serves a crafted Ubisoft OnlineConfig whose SandboxUrl points the RV
client at the local PRUDP server (rv_server) so that, once FCX login (condition A) advances,
the client's JobConnectEndPoint dials 127.0.0.1:<rv_port> instead of a dead Ubisoft host.

This is the config-seam half of the live capture (S0). It does NOT by itself make the game dial;
it only removes the "wrong host" variable. Expect zero inbound UDP until condition A is solved.

Run (admin only if you bind :80; default :8081 works unprivileged for local tests):
    python -I scripts/rv_server/config_server.py --port 8081 --rv-port 50000
Then redirect onlineconfigservice.ubi.com -> 127.0.0.1 in hosts (owner, admin) and, if hosts
maps to :80, bind this server with --port 80.
"""
from __future__ import annotations
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

CONFIG_TEMPLATE = [
    {"Name": "SandboxUrl", "Values": ["prudp://address=127.0.0.1;port=RVPORT"]},
    {"Name": "SandboxSecureUrl", "Values": ["prudp://address=127.0.0.1;port=RVPORT"]},
    {"Name": "TitleId", "Values": ["21fc0b75f83c49f2a51726f60fd1c5f8"]},
]


def build_config(rv_port: int) -> bytes:
    cfg = json.loads(json.dumps(CONFIG_TEMPLATE))  # deep copy
    for e in cfg:
        e["Values"] = [v.replace("RVPORT", str(rv_port)) for v in e["Values"]]
    return json.dumps(cfg).encode("utf-8")


class Handler(BaseHTTPRequestHandler):
    rv_port = 50000

    def do_GET(self):
        body = build_config(self.rv_port)
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        print(f"[config_server] {self.address_string()} {fmt % args}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8081, help="80 needs admin + hosts redirect")
    ap.add_argument("--rv-port", type=int, default=50000, help="rv_server PRUDP port to advertise")
    a = ap.parse_args()
    Handler.rv_port = a.rv_port
    print(f"config_server on http://{a.host}:{a.port}/ serving SandboxUrl -> prudp://address=127.0.0.1;port={a.rv_port}")
    print("redirect onlineconfigservice.ubi.com -> 127.0.0.1 in hosts (admin), then run the game;")
    print("the RV dial (if condition A advances) lands in rv_server's rv_capture.log.")
    ThreadingHTTPServer((a.host, a.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
