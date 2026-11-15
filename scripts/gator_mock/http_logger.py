# Local HTTP/80 logger that impersonates Ubisoft gconnect/onlineconfig so FC3's "go online"
# requests can be captured. Run from an elevated terminal (binding :80 needs admin), AFTER
# applying the hosts redirect (see hosts_patch.py). Logs every request to gator_http_log.txt
# next to this file. Usage:  python -I http_logger.py [port=80]
import http.server, socketserver, datetime, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
LOGF = os.path.join(HERE, "gator_http_log.txt")
CANNED = os.environ.get("GATOR_CANNED", "")  # optional path to a file with a canned response body

class Handler(http.server.BaseHTTPRequestHandler):
    def _handle(self):
        n = 0
        try: n = int(self.headers.get("Content-Length", "0") or 0)
        except Exception: n = 0
        body = self.rfile.read(n) if n else b""
        with open(LOGF, "a", encoding="utf-8") as f:
            f.write("\n" + "=" * 72 + "\n%s %s %s\n" % (datetime.datetime.now(), self.command, self.path))
            f.write("HEADERS:\n%s\n" % self.headers)
            f.write("BODY(%d):\n%s\n" % (len(body), body[:8192].decode("latin1")))
        print("%s %s" % (self.command, self.path))
        data = b""
        if CANNED and os.path.exists(CANNED):
            data = open(CANNED, "rb").read()
        self.send_response(200)
        self.send_header("Content-Type", "text/xml")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)
    def do_GET(self): self._handle()
    def do_POST(self): self._handle()
    def do_PUT(self): self._handle()
    def log_message(self, *a): pass

class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 80
    print("GATOR HTTP logger on 0.0.0.0:%d -> %s (Ctrl+C to stop)" % (port, LOGF))
    Server(("0.0.0.0", port), Handler).serve_forever()
