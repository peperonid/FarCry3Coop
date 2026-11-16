#!/usr/bin/env python3
"""test_prudp.py - offline proof that the PRUDP v1 codec round-trips and the RVServer
completes the SYN -> CONNECT handshake. Run: python -I scripts/rv_server/test_prudp.py
"""
import os, sys, socket, threading, time, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from prudp import (PRUDPPacket, opt, SYN, CONNECT, DATA, PING,
                   FLAG_RELIABLE, FLAG_NEED_ACK, OPT_UINT8, OPT_UINT32, MAGIC)
from rv_server import RVServer

def test_codec_roundtrip():
    pk = PRUDPPacket(type=SYN, flags=FLAG_NEED_ACK, session_id=1, substream_id=1,
                     sequence=7, source=0, destination=1,
                     options=[opt(0x01, 0xCA, OPT_UINT8), opt(0x02, 10000, OPT_UINT32),
                              opt(0x08, 0x1234, OPT_UINT32)],
                     payload=b"\x01\x02\x03hello")
    raw = pk.encode()
    assert raw[:2] == MAGIC
    back = PRUDPPacket.decode(raw)
    assert back.type == SYN and back.flags == pk.flags
    assert back.sequence == 7 and back.session_id == 1 and back.substream_id == 1
    assert [o.id for o in back.options] == [0x01, 0x02, 0x08]
    assert back.options[0].as_int() == 0xCA
    assert back.payload == b"\x01\x02\x03hello"
    print("codec round-trip: OK")

def test_handshake():
    port = 50123
    logp = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rv_capture_test.log")
    if os.path.exists(logp): os.remove(logp)
    srv = RVServer("127.0.0.1", port, logp)
    th = threading.Thread(target=srv.run, daemon=True); th.start()
    time.sleep(0.2)

    c = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); c.settimeout(1.0)
    dst = ("127.0.0.1", port)
    # 1) client SYN (connection request) with RV-ish options
    syn = PRUDPPacket(type=SYN, flags=FLAG_NEED_ACK, session_id=0, substream_id=0,
                      sequence=0, source=0, destination=1,
                      options=[opt(0x01, 0xCA, OPT_UINT8), opt(0x02, 20000, OPT_UINT32),
                               opt(0x04, 6, OPT_UINT8), opt(0x06, 1, OPT_UINT32)])
    c.sendto(syn.encode(), dst)
    data, _ = c.recvfrom(65535)
    r = PRUDPPacket.decode(data)
    assert r.type == CONNECT, f"expected CONNECT reply, got {r.type_name()}"
    cid = r.options and [o.as_int() for o in r.options if o.id == 0x08] or None
    print(f"handshake step1 SYN->CONNECT reply: OK (connid opt={cid})")
    # 2) client CONNECT -> server marks established
    con = PRUDPPacket(type=CONNECT, flags=FLAG_NEED_ACK, session_id=r.session_id,
                      substream_id=r.substream_id, sequence=r.sequence + 1,
                      source=0, destination=1,
                      options=[opt(0x01, 0xCA, OPT_UINT8), opt(0x08, cid[0] if cid else 0, OPT_UINT32)])
    c.sendto(con.encode(), dst)
    data, _ = c.recvfrom(65535)
    r2 = PRUDPPacket.decode(data)
    assert r2.type == CONNECT, "expected CONNECT ack"
    # server state should be established
    est = any(v["state"] == "established" for v in srv.conns.values())
    assert est, "server never reached established state"
    print("handshake step2 CONNECT->established: OK")
    # 3) RMC request over the established channel -> server ACK + RMC response
    sess = r2.session_id
    from prudp import RMCMessage, RMC_RESPONSE, RMC_REQUEST
    req = RMCMessage(RMC_REQUEST, protocol_id=0x0000, method_id=0x0000, session_id=7, body=b"ping")
    d = PRUDPPacket(type=DATA, flags=FLAG_RELIABLE | FLAG_NEED_ACK, session_id=sess,
                    substream_id=1, sequence=1, source=0, destination=1, payload=req.encode())
    c.sendto(d.encode(), dst)
    got_resp = False
    for _ in range(4):
        data, _ = c.recvfrom(65535)
        pk = PRUDPPacket.decode(data)
        if pk.type != DATA or not pk.payload:
            continue
        rm = RMCMessage.parse(pk.payload)
        assert rm.call_type == RMC_RESPONSE and rm.method_id == 0x0000, "bad RMC response"
        assert rm.session_id == 7
        got_resp = True
        break
    assert got_resp, "no RMC response received"
    print("RMC request->response over established channel: OK")
    srv.log.close(); c.close()

if __name__ == "__main__":
    test_codec_roundtrip()
    test_handshake()
    print("ALL OK")
