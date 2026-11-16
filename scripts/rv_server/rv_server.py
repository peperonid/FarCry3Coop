#!/usr/bin/env python3
"""rv_server.py - a minimal PRUDP v1 RendezVous emulator (condition B of the plan).

Stage-1 goal (owner ask): bind the host:port the game's RV client will dial, capture
every inbound PRUDP packet (raw hex + parsed fields), and answer the SYN -> CONNECT
handshake so the client advances. Payload encryption (prudps RC4/HMAC) is intentionally
out of scope here; capture the Connection Request first, then extend the codec.

Run:  python -I scripts/rv_server/rv_server.py [--host 127.0.0.1] [--port 50000]
Then point the served OnlineConfig SandboxUrl at prudp://address=127.0.0.1;port=50000.
When the game actually dials (needs condition A), the connection request lands in
the log here.
"""
from __future__ import annotations
import argparse
import errno
import os
import socket
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from prudp import (PRUDPPacket, Option, opt,
                   SYN, CONNECT, DATA, DISCONNECT, PING,
                   FLAG_ACK, FLAG_RELIABLE, FLAG_NEED_ACK,
                   OPT_UINT8, OPT_UINT16, OPT_UINT32)

def hexline(b: bytes, limit: int = 96) -> str:
    h = b.hex(" ")
    return h[:limit * 3] + (" ..." if len(b) > limit * 3 else "")

class RVServer:
    def __init__(self, host, port, logpath):
        self.host, self.port = host, port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((host, port))
        self.log = open(logpath, "a", buffering=1, encoding="utf-8")
        # connection registry keyed by (ip, port)
        self.conns = {}
        self.next_connid = 0x1000
        self.log.write(f"\n===== rv_server start {time.strftime('%Y-%m-%d %H:%M:%S')} on {host}:{port} =====\n")

    def logpkt(self, tag, addr, raw, pk=None):
        t = time.strftime("%H:%M:%S") + ".%03d" % int((time.time() % 1) * 1000)
        line = f"[{t}] {tag:9} {addr[0]}:{addr[1]} len={len(raw)} hex={hexline(raw)}"
        if pk is not None:
            line += (f" | {pk.type_name()} seq={pk.sequence} sess={pk.session_id} "
                     f"sub={pk.substream_id} src={pk.source} dst={pk.destination} "
                     f"flags=0x{pk.flags:x} opts={[(hex(o.id), o.size, o.raw.hex()) for o in pk.options]} "
                     f"payload={len(pk.payload)}b")
        self.log.write(line + "\n")
        print(line)

    def _opts(self, pk, oid):
        return [o for o in pk.options if o.id == oid]

    def handle(self, data, addr):
        try:
            pk = PRUDPPacket.decode(data)
        except Exception as e:
            self.logpkt("RAWDROP", addr, data)
            self.log.write(f"          parse error: {e}\n")
            return
        key = addr
        st = self.conns.setdefault(key, {"state": "new", "connid": None, "seq": 0})
        self.logpkt(pk.type_name(), addr, data, pk)

        if pk.type == SYN:
            st["connid"] = self.next_connid; self.next_connid += 1
            st["state"] = "syn_recv"
            # reply CONNECT with a connection id + echo RV options (magic, ping, max_outstanding, pid, cid)
            reply = PRUDPPacket(
                type=CONNECT,
                flags=FLAG_NEED_ACK | FLAG_ACK,
                session_id=pk.session_id, substream_id=pk.substream_id,
                sequence=pk.sequence + 1,
                source=pk.destination, destination=pk.source,
                options=self._reply_options(pk, st["connid"]),
            )
            self._send(reply, addr)

        elif pk.type == CONNECT:
            st["state"] = "established"
            self.log.write(f"          >>> CONNECTION ESTABLISHED {addr} connid=0x{st['connid']:x}\n")
            ack = PRUDPPacket(type=CONNECT, flags=FLAG_ACK,
                              session_id=pk.session_id, substream_id=pk.substream_id,
                              sequence=pk.sequence + 1, source=pk.destination, destination=pk.source,
                              options=[])
            self._send(ack, addr)

        elif pk.type == PING:
            pong = PRUDPPacket(type=PING, flags=FLAG_ACK,
                               session_id=pk.session_id, substream_id=pk.substream_id,
                               sequence=pk.sequence + 1, source=pk.destination, destination=pk.source)
            self._send(pong, addr)

        elif pk.type == DATA:
            # ack reliable data; payload RMC handling is a later stage
            if pk.has_flag(FLAG_NEED_ACK):
                ack = PRUDPPacket(type=DATA, flags=FLAG_ACK,
                                  session_id=pk.session_id, substream_id=pk.substream_id,
                                  sequence=pk.sequence + 1, source=pk.destination, destination=pk.source)
                self._send(ack, addr)

        elif pk.type == DISCONNECT:
            self.log.write(f"          <<< DISCONNECT {addr}\n")

    def _reply_options(self, pk, connid):
        # Echo the RV negotiation options the client offered (so it accepts them), then
        # inject/replace the connection id. We keep unknown option IDs verbatim (safety).
        seen = set()
        out = []
        for o in pk.options:
            out.append(o); seen.add(o.id)
        # connection id option (id 0x08 in the NEX/RV table; adjust once a live capture confirms it)
        if 0x08 not in seen:
            out.append(opt(0x08, connid, OPT_UINT32))
        return out

    def _send(self, pk, addr):
        raw = pk.encode(with_signature=False)
        self.sock.sendto(raw, addr)
        self.logpkt(f"SEND:{pk.type_name()}", addr, raw, pk)

    def run(self):
        print(f"rv_server listening udp {self.host}:{self.port} -> log {self.log.name}")
        while True:
            data, addr = self.sock.recvfrom(65535)
            try:
                self.handle(data, addr)
            except Exception as e:
                self.log.write(f"handle error: {e}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=50000)
    ap.add_argument("--log", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "rv_capture.log"))
    a = ap.parse_args()
    RVServer(a.host, a.port, a.log).run()


if __name__ == "__main__":
    main()
