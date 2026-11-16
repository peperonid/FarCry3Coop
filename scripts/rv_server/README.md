# rv_server - PRUDP/RendezVous emulator (condition B of the co-op plan)

> Bilingual: English here - [русская версия](README.ru.md)
> Status: **Stage S1 done** (offline PRUDP v1 codec + handshake). **Stage S0 (live capture) is
> gated on condition (A)** - the game only dials RV once FCX reaches `LoggingIn` (NOTES 28cw/28cv).

This is the local RendezVous (RV) / PRUDP emulator: the peer the two FC3 clients would
rendezvous through so their P2P co-op netcode can connect over the LAN/loopback. It is
condition **B** in `docs/coop-gator-emulation-plan.md`; the head blocker is still condition
**A** (FCX online login), which is upstream of this server.

## Files
- `prudp.py` - PRUDP **v1** (RendezVous variation) codec: `PRUDPPacket.decode/encode` + options
  `id,(type<<4|size),value`. Little endian. Tolerant parser keeps every option verbatim.
- `rv_server.py` - UDP server with a `SYN -> CONNECT -> (CONNECT ack) -> DATA` handshake state
  machine and a hex log of every packet (`rv_capture.log`). Echoes the client's RV options and
  injects a connection-id option so the client should accept the reply.
- `test_prudp.py` - offline proof: codec round-trip + a synthetic PRUDP client that completes the
  handshake against the server over UDP loopback. Prints `ALL OK`.

## Run
```
python -I scripts/rv_server/test_prudp.py          # self-test (offline), prints ALL OK
python -I scripts/rv_server/rv_server.py --host 127.0.0.1 --port 50000
```
Then serve an OnlineConfig whose `SandboxUrl` is `prudp://address=127.0.0.1;port=50000`
(the config seam, NOTES 28bs). Use the plain `prudp` scheme (not `prudps`) to avoid payload
encryption during bring-up capture.

## What works now (offline, no game, no C:)
- PRUDP v1 packet encode/decode (SYN/CONNECT/DATA/PING/DISCONNECT, options, payload).
- Server accepts a Connection Request (SYN), replies CONNECT with a connection id, completes to
  `established`, ACKs PING and reliable DATA. Full hex capture to `rv_capture.log`.

## Not yet (next stages)
- **S0 live capture**: needs condition (A) so the real FC3 client dials us; until then the
  synthetic client exercises the same path.
- **prudps** (RC4 + HMAC payload encryption/signature): FC3 may use it; the header checksum in
  `prudp.py` is a placeholder to be fixed against a real capture.
- **Real RV option IDs**: the connection-id / pid / cid option numbers are the NEX-table guesses;
  confirm against a live Connection Request and fix.
- **RMC / RV service messages** after connect (the actual create/join/get-ticket calls): the
  protocol layer (`.mg`? no - this is Quazal RMC over PRUDP) is the following milestone.

## Honesty
This proves the transport/handshake engine works and is testable without the game. It does NOT
yet mean two games connect - that still requires condition (A) (FCX login) plus the RMC service
layer on top of the handshake. Treat S1 as the foundation, not the finish.
