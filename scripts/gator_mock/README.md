# gator_mock - intercept FC3's online bootstrap (A2 first layer)

FC3 (1.05 binaries) dials `gconnect.ubi.com` over **plaintext HTTP on port 80** to check the
online service; today that host is alive but returns a "service unavailable" state, so the game
shows "Far Cry 3 service is not available". This tool captures those exact requests so we can
learn the protocol and answer it (A2: local GATOR/gconnect emulator; see docs/coop-gator-emulation-plan.md).

How the game's online path behaves (NOTES 28br): TCP to gconnect.ubi.com:80 (HTTP), then it binds
UDP 9000 (Storm engine) + 3074 (GATOR/console); no session forms because gconnect returns "down".

Steps (all from an **elevated** terminal - hosts file + port 80 need admin):
1. `python -I hosts_patch.py apply`      # backs up hosts, points gconnect/onlineconfig to 127.0.0.1
2. `python -I http_logger.py`            # starts a logging HTTP server on 0.0.0.0:80 (leave running)
3. Launch FC3 normally, go Multiplayer -> Co-op -> try to create/host a private session.
4. Read `gator_http_log.txt` (next to these files) - it records every method/path/header/body.
5. `python -I hosts_patch.py restore`    # put hosts back when done

Next after capture: craft canned responses (set GATOR_CANNED=<file>) to get past the "unavailable"
gate, then tackle the Quazal RV/PRUDP UDP layer (ports 3074/9000) where the crypto lives.
