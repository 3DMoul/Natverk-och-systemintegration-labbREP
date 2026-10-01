#!/usr/bin/env python3
"""Lokal API-server med reproducerbara svar för robusthetsövningar."""

import argparse
import json
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse


attempts = {}


class Handler(BaseHTTPRequestHandler):
    def log_message(self, _format, *_args):
        return

    def send_data(self, status, body, content_type="application/json", headers=None):
        encoded = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(encoded)))
        for name, value in (headers or {}).items():
            self.send_header(name, value)
        self.end_headers()
        try:
            self.wfile.write(encoded)
        except BrokenPipeError:
            pass

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/health":
            self.send_data(200, '{"status":"ok"}')
            return
        if parsed.path != "/api/config":
            self.send_data(404, '{"code":"not_found","message":"Unknown resource"}')
            return

        query = parse_qs(parsed.query)
        scenario = query.get("scenario", ["ok"])[0]
        request_id = self.headers.get("X-Request-ID", "missing")
        key = (scenario, request_id)
        attempts[key] = attempts.get(key, 0) + 1
        attempt = attempts[key]
        print(json.dumps({"request_id": request_id, "scenario": scenario,
                          "attempt": attempt}, ensure_ascii=False), flush=True)

        if scenario == "bad-request":
            self.send_data(400, '{"code":"invalid_request","message":"Request is invalid"}')
        elif scenario == "unauthorized":
            self.send_data(401, '{"code":"unauthorized","message":"Authentication required"}')
        elif scenario == "rate-limit" and attempt == 1:
            self.send_data(429, '{"code":"rate_limited","message":"Try later"}',
                           headers={"Retry-After": "1"})
        elif scenario == "flaky" and attempt == 1:
            self.send_data(500, '{"code":"temporary_failure","message":"Try again"}')
        elif scenario == "bad-json":
            self.send_data(200, '{"sensor_id":"temperature-1", broken')
        elif scenario == "wrong-type":
            self.send_data(200, '{"sensor_id":"temperature-1","value":"warm","unit":"C"}')
        elif scenario == "slow":
            time.sleep(1.5)
            self.send_data(200, '{"sensor_id":"temperature-1","value":21.5,"unit":"C"}')
        else:
            self.send_data(200, '{"sensor_id":"temperature-1","value":21.5,"unit":"C"}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8093)
    args = parser.parse_args()
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"Server: http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
