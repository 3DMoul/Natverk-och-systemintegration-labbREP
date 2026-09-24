#!/usr/bin/env python3
"""Mätklient som rapporterar svarstid och felklass utan externa paket."""

import argparse
import json
import math
import statistics
import time
import urllib.error
import urllib.request


def percentile_nearest_rank(values, percentile):
    ordered = sorted(values)
    rank = max(1, math.ceil(percentile / 100 * len(ordered)))
    return ordered[rank - 1]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8091)
    parser.add_argument("--count", type=int, default=20)
    parser.add_argument("--interval-ms", type=int, default=50)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("--invalid-every", type=int, default=0)
    return parser.parse_args()


def main():
    args = parse_args()
    url = f"http://{args.host}:{args.port}/api/readings"
    durations = []
    successes = 0
    errors = {}
    run_start = time.perf_counter()
    for number in range(1, args.count + 1):
        request_id = f"day10-{number:03d}"
        value = "invalid" if args.invalid_every and number % args.invalid_every == 0 else 20.0 + number / 10
        data = json.dumps({"sensor_id": "sim-1", "value": value, "unit": "C"}).encode()
        request = urllib.request.Request(url, data=data, method="POST",
                                         headers={"Content-Type": "application/json",
                                                  "X-Request-ID": request_id})
        start = time.perf_counter()
        status = None
        classification = "ok"
        try:
            with urllib.request.urlopen(request, timeout=args.timeout) as response:
                status = response.status
                response.read()
                successes += 1
        except urllib.error.HTTPError as error:
            status = error.code
            error.read()
            classification = f"http_{status}"
        except (urllib.error.URLError, TimeoutError) as error:
            classification = "connection_or_timeout"
            print(f"{request_id} error={error}")
        duration = (time.perf_counter() - start) * 1000
        durations.append(duration)
        if classification != "ok":
            errors[classification] = errors.get(classification, 0) + 1
        print(f"{request_id} status={status} class={classification} duration_ms={duration:.3f}")
        if number < args.count:
            time.sleep(args.interval_ms / 1000)
    elapsed = time.perf_counter() - run_start
    failures = args.count - successes
    summary = {
        "attempts": args.count,
        "successes": successes,
        "failures": failures,
        "error_fraction": round(failures / args.count, 4),
        "elapsed_s": round(elapsed, 3),
        "throughput_success_per_s": round(successes / elapsed, 3),
        "latency_ms_min": round(min(durations), 3),
        "latency_ms_avg": round(statistics.fmean(durations), 3),
        "latency_ms_p95": round(percentile_nearest_rank(durations, 95), 3),
        "latency_ms_max": round(max(durations), 3),
        "errors": errors,
    }
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
