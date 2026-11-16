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
  handshake AND an RMC request->response over the established channel. Prints `ALL OK`.
- `config_server.py` - serves a crafted OnlineConfig with `SandboxUrl -> prudp://address=127.0.0.1;port=50000`
  (the config seam) so the client, once it dials, hits rv_server.
- `../run_a2_rvcapture.bat` - one-shot harness: starts rv_server (:50000) + config_server (:8081)
  and prints the owner admin steps (hosts redirect + game launch).

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

## RMC layer (scaffolding, offline-tested)
After the handshake, reliable DATA payloads are parsed as `RMCMessage(call_type, protocol_id,
method_id, session_id, body)` and dispatched through `RVServer.services` keyed by
`(protocol_id, method_id)`. A synthetic client does a request->response round trip in the test.
This layout is OUR self-consistent framing; the real Quazal-RendezVous RMC encoding and the RV
protocol/method IDs must be confirmed from a live capture (S0) before the handlers are meaningful.

## Not yet (next stages)
- **S0 live capture**: needs condition (A) so the real FC3 client dials us; until then the
  synthetic client exercises the same path. Harness: `../run_a2_rvcapture.bat` (rv_server +
  config_server + hosts step). Expected result today: **zero inbound UDP** = clean proof the gate
  is FCX login (condition A), not the socket/config layer.
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
