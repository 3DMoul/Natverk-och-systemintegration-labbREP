#!/usr/bin/env python3
"""Skicka reproducerbara sensorscenarier till dag 9-servern."""

from __future__ import annotations

import argparse
import json
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


SCENARIOS = [
    ("demo-temp-1", "/api/readings", {"sensor_id": "temperature-1", "value": 21.5, "unit": "C"}),
    ("demo-humidity-1", "/api/readings", {"sensor_id": "humidity-1", "value": 47.2, "unit": "%"}),
    ("demo-invalid-1", "/api/readings", {"sensor_id": "temperature-1", "value": "warm", "unit": "C"}),
    ("demo-missing-1", "/api/unknown", {"sensor_id": "temperature-1", "value": 22.0, "unit": "C"}),
]


def send(base_url: str, request_id: str, path: str, body: dict[str, Any]) -> int:
    payload = json.dumps(body).encode("utf-8")
    request = Request(
        base_url + path,
        data=payload,
        method="POST",
        headers={"Content-Type": "application/json", "X-Request-ID": request_id},
    )
    try:
        with urlopen(request, timeout=3) as response:
            response_body = response.read().decode("utf-8")
            echoed_id = response.headers.get("X-Request-ID")
            print(f"CLIENT request_id={request_id} status={response.status} echoed_id={echoed_id} body={response_body}")
            return response.status
    except HTTPError as error:
        response_body = error.read().decode("utf-8")
        echoed_id = error.headers.get("X-Request-ID")
        print(f"CLIENT request_id={request_id} status={error.code} echoed_id={echoed_id} body={response_body}")
        return error.code


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8090)
    args = parser.parse_args()
    base_url = f"http://{args.host}:{args.port}"

    try:
        statuses = [send(base_url, *scenario) for scenario in SCENARIOS]
    except URLError as error:
        raise SystemExit(f"Kan inte ansluta till {base_url}: {error.reason}") from error

    expected = [202, 202, 400, 404]
    if statuses != expected:
        raise SystemExit(f"FAIL statuses={statuses} expected={expected}")
    print(f"RESULT statuses={statuses} expected={expected}")


if __name__ == "__main__":
    main()
