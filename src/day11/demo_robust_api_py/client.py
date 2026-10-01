#!/usr/bin/env python3
"""Robust referensklient med explicit fel- och retry-policy."""

import argparse
import json
import time
import urllib.error
import urllib.request


RETRYABLE_STATUS = {429, 500, 502, 503, 504}


def validate_config(data):
    if not isinstance(data, dict):
        raise ValueError("response must be an object")
    if not isinstance(data.get("sensor_id"), str):
        raise ValueError("sensor_id must be a string")
    value = data.get("value")
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        raise ValueError("value must be a number")
    if not isinstance(data.get("unit"), str):
        raise ValueError("unit must be a string")


def run(scenario, host, port, timeout, max_attempts):
    request_id = f"day11-{scenario}"
    url = f"http://{host}:{port}/api/config?scenario={scenario}"
    for attempt in range(1, max_attempts + 1):
        request = urllib.request.Request(url, headers={
            "Accept": "application/json",
            "Authorization": "Bearer classroom-placeholder",
            "X-Request-ID": request_id,
        })
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                content_type = response.headers.get_content_type()
                body = response.read().decode("utf-8")
                if content_type != "application/json":
                    raise ValueError(f"unexpected Content-Type: {content_type}")
                data = json.loads(body)
                validate_config(data)
                print(f"attempt={attempt} status={response.status} decision=success")
                return 0
        except urllib.error.HTTPError as error:
            status = error.code
            retryable = status in RETRYABLE_STATUS and attempt < max_attempts
            decision = "retry" if retryable else "stop"
            print(f"attempt={attempt} status={status} decision={decision}")
            if not retryable:
                return 1
            retry_after = error.headers.get("Retry-After")
            wait = min(float(retry_after), 2.0) if retry_after else 0.1 * (2 ** (attempt - 1))
            print(f"wait_seconds={wait:.3f}")
            time.sleep(wait)
        except (urllib.error.URLError, TimeoutError) as error:
            print(f"attempt={attempt} error=timeout_or_connection decision=stop detail={error}")
            return 1
        except (json.JSONDecodeError, ValueError) as error:
            print(f"attempt={attempt} error=contract decision=stop detail={error}")
            return 1
    return 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scenario", choices=["ok", "bad-request", "unauthorized",
                                             "rate-limit", "flaky", "bad-json",
                                             "wrong-type", "slow"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8093)
    parser.add_argument("--timeout", type=float, default=0.4)
    parser.add_argument("--max-attempts", type=int, default=3)
    args = parser.parse_args()
    raise SystemExit(run(args.scenario, args.host, args.port,
                         args.timeout, args.max_attempts))


if __name__ == "__main__":
    main()
