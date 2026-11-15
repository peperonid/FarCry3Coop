# run_capture.py - one elevated process that BOTH answers FC3's online-config request with our
# crafted fc3_config.json AND sniffs the Quazal RV / PRUDP packets FC3 then sends to the local
# endpoint. Run elevated (needs admin for :80 and binding the UDP port). Ctrl+C to stop.
#   python -I run_capture.py            # HTTP :80 + UDP 127.0.0.1:50000
#   python -I run_capture.py 80 50000   # custom ports
import http.server, socketserver, socket, threading, datetime, os, sys, binascii
HERE = os.path.dirname(os.path.abspath(__file__))
HTTP_LOG = os.path.join(HERE, "gator_http_log.txt")
UDP_LOG  = os.path.join(HERE, "gator_udp_log.txt")
CFG      = os.path.join(HERE, "fc3_config.json")

def cfg_bytes():
    try: return open(CFG, "rb").read()
    except Exception: return b"[]"

class HTTPHandler(http.server.BaseHTTPRequestHandler):
    def _h(self):
        n = int(self.headers.get("Content-Length", "0") or 0)
        body = self.rfile.read(n) if n else b""
        with open(HTTP_LOG, "a", encoding="utf-8") as f:
            f.write("\n" + "="*72 + "\n%s %s %s\nHEADERS:\n%s\nBODY(%d):\n%s\n" % (
                datetime.datetime.now(), self.command, self.path, self.headers, len(body),
                body[:8192].decode("latin1")))
        data = cfg_bytes() if "GetOnlineConfig" in self.path else b""
        print("[HTTP] %s %s -> %d bytes" % (self.command, self.path, len(data)))
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)
    do_GET = _h; do_POST = _h; do_PUT = _h
    def log_message(self, *a): pass

class Srv(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

def udp_sniffer(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("127.0.0.1", port))
    print("[UDP] listening on 127.0.0.1:%d" % port)
    while True:
        data, addr = s.recvfrom(65535)
        ts = datetime.datetime.now()
        with open(UDP_LOG, "a", encoding="utf-8") as f:
            f.write("\n%s from %s:%s len=%d\n%s\n" % (ts, addr[0], addr[1], len(data),
                    binascii.hexlify(data).decode()))
        print("[UDP] %d bytes from %s:%s  %s..." % (len(data), addr[0], addr[1],
              binascii.hexlify(data[:24]).decode()))

if __name__ == "__main__":
    http_port = int(sys.argv[1]) if len(sys.argv) > 1 else 80
    udp_port  = int(sys.argv[2]) if len(sys.argv) > 2 else 50000
    print("FC3 gator capture: HTTP :%d serving %s ; UDP 127.0.0.1:%d (Ctrl+C to stop)" % (http_port, CFG, udp_port))
    threading.Thread(target=udp_sniffer, args=(udp_port,), daemon=True).start()
    Srv(("0.0.0.0", http_port), HTTPHandler).serve_forever()
