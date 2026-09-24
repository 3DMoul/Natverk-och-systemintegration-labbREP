#!/usr/bin/env python3
"""Lokal HTTP-server med kontrollerbar fördröjning och serverfel."""

import argparse
import json
import math
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class State:
    def __init__(self, delay_ms: int, failure_every: int):
        self.delay_ms = delay_ms
        self.failure_every = failure_every
        self.requests = 0
        self.accepted = 0
        self.validation_errors = 0
        self.server_errors = 0
        self.total_processing_ms = 0.0
        self.max_processing_ms = 0.0


def log_event(**fields):
    fields = {"timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), **fields}
    print(json.dumps(fields, ensure_ascii=False), flush=True)


class Handler(BaseHTTPRequestHandler):
    state: State

    def log_message(self, _format, *_args):
        return

    def send_json(self, status, payload):
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"status": "ok"})
            return
        if self.path == "/metrics":
            s = self.state
            avg = s.total_processing_ms / s.requests if s.requests else 0.0
            self.send_json(200, {
                "requests_total": s.requests,
                "accepted_total": s.accepted,
                "validation_errors_total": s.validation_errors,
                "server_errors_total": s.server_errors,
                "processing_ms_avg": round(avg, 3),
                "processing_ms_max": round(s.max_processing_ms, 3),
            })
            return
        self.send_json(404, {"error": "not_found"})

    def do_POST(self):
        if self.path != "/api/readings":
            self.send_json(404, {"error": "not_found"})
            return
        start = time.perf_counter()
        self.state.requests += 1
        number = self.state.requests
        request_id = self.headers.get("X-Request-ID", f"server-{number}")
        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length))
        except (ValueError, json.JSONDecodeError):
            self.state.validation_errors += 1
            self.finish_request(start, request_id, 400, "invalid_json")
            return
        value = payload.get("value")
        if not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value):
            self.state.validation_errors += 1
            self.finish_request(start, request_id, 400, "value_must_be_finite_number")
            return
        if self.state.delay_ms:
            time.sleep(self.state.delay_ms / 1000)
        if self.state.failure_every and number % self.state.failure_every == 0:
            self.state.server_errors += 1
            self.finish_request(start, request_id, 500, "simulated_server_failure")
            return
        self.state.accepted += 1
        self.finish_request(start, request_id, 202, "accepted")

    def finish_request(self, start, request_id, status, event):
        duration = (time.perf_counter() - start) * 1000
        self.state.total_processing_ms += duration
        self.state.max_processing_ms = max(self.state.max_processing_ms, duration)
        log_event(level="INFO" if status < 400 else "WARNING", event=event,
                  request_id=request_id, status=status,
                  processing_ms=round(duration, 3))
        self.send_json(status, {"event": event, "request_id": request_id,
                                "processing_ms": round(duration, 3)})


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8091)
    parser.add_argument("--delay-ms", type=int, default=0)
    parser.add_argument("--failure-every", type=int, default=0)
    return parser.parse_args()


def main():
    args = parse_args()
    Handler.state = State(args.delay_ms, args.failure_every)
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"Server: http://{args.host}:{args.port} delay_ms={args.delay_ms} failure_every={args.failure_every}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
