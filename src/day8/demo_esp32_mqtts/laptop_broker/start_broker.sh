#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

for required_file in generated/ca.crt generated/server.crt generated/server.key generated/passwords; do
  if ! test -f "$required_file"; then
    echo "MISSING: $required_file"
    echo "Run ./prepare_broker.sh LAPTOP_LAN_IP first."
    exit 2
  fi
done

echo "Starting the classroom broker on MQTTS port 8883. Stop with Ctrl+C."
exec mosquitto -c mosquitto.conf -v
