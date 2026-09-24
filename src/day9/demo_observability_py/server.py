#!/usr/bin/env python3
"""Lokal undervisningsserver för strukturerad loggning och mätetal."""

from __future__ import annotations

import argparse
import json
import math
import threading
import time
import uuid
from datetime import datetime, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any
from urllib.parse import urlsplit


DASHBOARD = """<!doctype html>
<html lang="sv">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>IoT25 – lokal dashboard</title>
  <style>
    body { font: 16px system-ui, sans-serif; margin: 2rem; background: #f4f7f9; color: #17222b; }
    h1 { margin-bottom: .25rem; }
    #status { color: #52616b; }
    main { display: grid; grid-template-columns: repeat(auto-fit, minmax(13rem, 1fr)); gap: 1rem; }
    article { background: white; border-left: .4rem solid #167d8d; border-radius: .3rem; padding: 1rem; box-shadow: 0 .1rem .5rem #0002; }
    article strong { display: block; font-size: 1.7rem; margin-top: .4rem; }
    code { font-size: .85rem; }
  </style>
</head>
<body>
  <h1>IoT-flödets mätetal</h1>
  <p id="status">Hämtar…</p>
  <main id="cards"></main>
  <script>
    const shown = [
      ["http_requests_total", "HTTP-anrop", "st"],
      ["readings_accepted_total", "Accepterade mätningar", "st"],
      ["validation_errors_total", "Valideringsfel", "st"],
      ["not_found_total", "Okända resurser", "st"],
      ["request_duration_ms_avg", "Genomsnittlig svarstid", "ms"],
      ["request_duration_ms_max", "Maximal svarstid", "ms"],
      ["uptime_seconds", "Upptid", "s"]
    ];
    async function refresh() {
      try {
        const response = await fetch('/api/metrics', {cache: 'no-store'});
        const data = await response.json();
        document.getElementById('cards').innerHTML = shown.map(([key, title, unit]) =>
          `<article><code>${key}</code><strong>${data[key]} ${unit}</strong><span>${title}</span></article>`
        ).join('');
        document.getElementById('status').textContent = `Senast uppdaterad ${new Date().toLocaleTimeString()} · sidan hämtar samma data varannan sekund`;
      } catch (error) {
        document.getElementById('status').textContent = `Kunde inte hämta mätetal: ${error}`;
      }
    }
    refresh();
    setInterval(refresh, 2000);
  </script>
</body>
</html>
"""


def utc_timestamp() -> str:
    """Returnera en UTC-tidsstämpel som kan korreleras mellan komponenter."""
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


def log_event(level: str, event: str, **fields: Any) -> None:
    """Skriv exakt ett JSON-objekt per rad så att loggen kan filtreras."""
    record = {"timestamp": utc_timestamp(), "level": level, "event": event, **fields}
    print(json.dumps(record, ensure_ascii=False, separators=(",", ":")), flush=True)


class Metrics:
    """Trådsäkert tillstånd eftersom HTTP-servern kan hantera flera anrop samtidigt."""

    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.started = time.monotonic()
        self.http_requests_total = 0
        self.readings_accepted_total = 0
        self.validation_errors_total = 0
        self.not_found_total = 0
        self.duration_ms_sum = 0.0
        self.duration_ms_max = 0.0

    def complete_request(self, duration_ms: float) -> None:
        with self.lock:
            self.http_requests_total += 1
            self.duration_ms_sum += duration_ms
            self.duration_ms_max = max(self.duration_ms_max, duration_ms)

    def increment(self, name: str) -> None:
        with self.lock:
            setattr(self, name, getattr(self, name) + 1)

    def snapshot(self) -> dict[str, int | float]:
        with self.lock:
            count = self.http_requests_total
            average = self.duration_ms_sum / count if count else 0.0
            return {
                "http_requests_total": count,
                "readings_accepted_total": self.readings_accepted_total,
                "validation_errors_total": self.validation_errors_total,
                "not_found_total": self.not_found_total,
                "request_duration_ms_avg": round(average, 3),
                "request_duration_ms_max": round(self.duration_ms_max, 3),
                "uptime_seconds": round(time.monotonic() - self.started, 1),
            }


METRICS = Metrics()


class DemoHandler(BaseHTTPRequestHandler):
    server_version = "IoT25Observability/1.0"

    def log_message(self, format: str, *args: Any) -> None:
        # Stäng av BaseHTTPRequestHandlers fria textlogg; alla våra loggar är JSON.
        return

    def request_id(self) -> str:
        return self.headers.get("X-Request-ID") or f"server-{uuid.uuid4()}"

    def send_payload(
        self,
        status: HTTPStatus,
        payload: bytes,
        content_type: str,
        request_id: str,
    ) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Request-ID", request_id)
        self.end_headers()
        self.wfile.write(payload)

    def send_json(self, status: HTTPStatus, body: dict[str, Any], request_id: str) -> None:
        payload = json.dumps(body, ensure_ascii=False).encode("utf-8")
        self.send_payload(status, payload, "application/json; charset=utf-8", request_id)

    def finish_observation(
        self,
        started: float,
        request_id: str,
        status: HTTPStatus,
        event: str,
        level: str = "INFO",
        **fields: Any,
    ) -> None:
        # Monoton klocka används för duration eftersom väggklockan kan justeras.
        duration_ms = (time.monotonic() - started) * 1000
        METRICS.complete_request(duration_ms)
        log_event(
            level,
            event,
            request_id=request_id,
            method=self.command,
            path=urlsplit(self.path).path,
            status=int(status),
            duration_ms=round(duration_ms, 3),
            **fields,
        )

    def do_GET(self) -> None:
        started = time.monotonic()
        request_id = self.request_id()
        path = urlsplit(self.path).path

        if path == "/health":
            status = HTTPStatus.OK
            self.send_json(status, {"status": "ok"}, request_id)
            self.finish_observation(started, request_id, status, "health_checked")
            return

        if path == "/api/metrics":
            # Snapshoten tas före detta anrop räknas; nästa hämtning visar anropet.
            status = HTTPStatus.OK
            self.send_json(status, METRICS.snapshot(), request_id)
            self.finish_observation(started, request_id, status, "metrics_read")
            return

        if path == "/metrics":
            snapshot = METRICS.snapshot()
            lines = [f"iot25_{name} {value}" for name, value in snapshot.items()]
            payload = ("\n".join(lines) + "\n").encode("utf-8")
            status = HTTPStatus.OK
            self.send_payload(status, payload, "text/plain; charset=utf-8", request_id)
            self.finish_observation(started, request_id, status, "metrics_read")
            return

        if path in ("/", "/dashboard"):
            status = HTTPStatus.OK
            self.send_payload(status, DASHBOARD.encode("utf-8"), "text/html; charset=utf-8", request_id)
            self.finish_observation(started, request_id, status, "dashboard_read")
            return

        METRICS.increment("not_found_total")
        status = HTTPStatus.NOT_FOUND
        self.send_json(status, {"error": "not found"}, request_id)
        self.finish_observation(started, request_id, status, "route_not_found", "WARNING")

    def do_POST(self) -> None:
        started = time.monotonic()
        request_id = self.request_id()
        path = urlsplit(self.path).path

        if path != "/api/readings":
            METRICS.increment("not_found_total")
            status = HTTPStatus.NOT_FOUND
            self.send_json(status, {"error": "not found"}, request_id)
            self.finish_observation(started, request_id, status, "route_not_found", "WARNING")
            return

        try:
            content_length = int(self.headers.get("Content-Length", "0"))
            if content_length <= 0 or content_length > 16_384:
                raise ValueError("body size must be between 1 and 16384 bytes")
            body = json.loads(self.rfile.read(content_length))
            sensor_id = body.get("sensor_id")
            value = body.get("value")
            unit = body.get("unit")
            if not isinstance(sensor_id, str) or not sensor_id.strip():
                raise ValueError("sensor_id must be a non-empty string")
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError("value must be a finite number")
            if not isinstance(unit, str) or not unit.strip():
                raise ValueError("unit must be a non-empty string")
        except (json.JSONDecodeError, UnicodeDecodeError, ValueError, AttributeError) as error:
            METRICS.increment("validation_errors_total")
            status = HTTPStatus.BAD_REQUEST
            self.send_json(status, {"error": str(error)}, request_id)
            self.finish_observation(
                started,
                request_id,
                status,
                "reading_rejected",
                "WARNING",
                reason=str(error),
            )
            return

        METRICS.increment("readings_accepted_total")
        status = HTTPStatus.ACCEPTED
        self.send_json(status, {"status": "accepted", "request_id": request_id}, request_id)
        self.finish_observation(
            started,
            request_id,
            status,
            "reading_accepted",
            sensor_id=sensor_id,
            unit=unit,
        )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8090)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), DemoHandler)
    actual_host, actual_port = server.server_address
    log_event("INFO", "server_started", host=actual_host, port=actual_port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log_event("INFO", "server_stopping")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
