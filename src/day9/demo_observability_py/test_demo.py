#!/usr/bin/env python3
"""Automatisk kontroll av dag 9-demon utan externa paket."""

from __future__ import annotations

import json
import subprocess
import sys
import time
from pathlib import Path
from urllib.request import Request, urlopen


HERE = Path(__file__).resolve().parent


def get_json(url: str) -> tuple[dict[str, object], str | None]:
    request = Request(url, headers={"X-Request-ID": "test-metrics-1"})
    with urlopen(request, timeout=2) as response:
        return json.load(response), response.headers.get("X-Request-ID")


def main() -> None:
    server = subprocess.Popen(
        [sys.executable, str(HERE / "server.py"), "--port", "0"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    assert server.stdout is not None
    try:
        first_line = server.stdout.readline()
        started = json.loads(first_line)
        port = int(started["port"])
        base_url = f"http://127.0.0.1:{port}"

        client = subprocess.run(
            [sys.executable, str(HERE / "client.py"), "--port", str(port)],
            check=True,
            capture_output=True,
            text=True,
            timeout=10,
        )
        metrics, echoed_id = get_json(base_url + "/api/metrics")
        assert metrics["readings_accepted_total"] == 2, metrics
        assert metrics["validation_errors_total"] == 1, metrics
        assert metrics["not_found_total"] == 1, metrics
        assert metrics["http_requests_total"] == 4, metrics
        assert echoed_id == "test-metrics-1", echoed_id
        assert "RESULT statuses=[202, 202, 400, 404]" in client.stdout
        print("PASS: scenarier, domänräknare och X-Request-ID verifierade")
    finally:
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=3)
        time.sleep(0.05)


if __name__ == "__main__":
    main()
