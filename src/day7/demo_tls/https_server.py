#!/usr/bin/env python3
import json
import os
import ssl
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = "127.0.0.1"
PORT = 8443
EXPECTED_KEY = os.environ.get("IOT25_API_KEY", "")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/api/readings/latest":
            self.send_error(404)
            return

        if not EXPECTED_KEY or self.headers.get("X-API-Key") != EXPECTED_KEY:
            self.send_response(401)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error":"unauthorized"}')
            print("AUTH denied", flush=True)
            return

        payload = json.dumps({
            "sensorId": "room-a-temp-01",
            "value": 21.7,
            "unit": "C"
        }).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)
        print("AUTH accepted", flush=True)

    def log_message(self, format, *args):
        print("HTTP", format % args, flush=True)


def main():
    if not EXPECTED_KEY:
        raise SystemExit("IOT25_API_KEY must be set")

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain("generated/server.crt", "generated/server.key")

    server = ThreadingHTTPServer((HOST, PORT), Handler)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    print(f"HTTPS listening on https://localhost:{PORT}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
