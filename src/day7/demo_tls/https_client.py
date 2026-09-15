#!/usr/bin/env python3
import argparse
import ssl
import urllib.error
import urllib.request


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--trust-ca", action="store_true")
    parser.add_argument("--api-key")
    args = parser.parse_args()

    context = ssl.create_default_context(
        cafile="generated/ca.crt" if args.trust_ca else None
    )
    headers = {}
    if args.api_key:
        headers["X-API-Key"] = args.api_key

    request = urllib.request.Request(
        "https://localhost:8443/api/readings/latest",
        headers=headers
    )

    try:
        with urllib.request.urlopen(request, context=context, timeout=3) as response:
            print(f"HTTP {response.status} {response.read().decode('utf-8')}")
            return 0
    except urllib.error.HTTPError as error:
        print(f"HTTP {error.code} {error.read().decode('utf-8')}")
        return 10
    except urllib.error.URLError as error:
        if isinstance(error.reason, ssl.SSLCertVerificationError):
            print("TLS_VERIFY_FAILED certificate authority is not trusted")
            return 20
        print(f"TRANSPORT_ERROR {error.reason}")
        return 30


if __name__ == "__main__":
    raise SystemExit(main())
