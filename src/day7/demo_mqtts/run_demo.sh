#!/usr/bin/env bash
set -u

cd "$(dirname "$0")" || exit 1

for command_name in openssl mosquitto mosquitto_pub mosquitto_sub; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "MISSING: $command_name"
    exit 2
  fi
done

bash generate_certificates.sh || exit 1
mosquitto -c mosquitto.conf >broker.log 2>&1 &
broker_pid=$!
subscriber_pid=""
cleanup() {
  if test -n "$subscriber_pid"; then
    kill "$subscriber_pid" 2>/dev/null || true
    wait "$subscriber_pid" 2>/dev/null || true
  fi
  kill "$broker_pid" 2>/dev/null || true
  wait "$broker_pid" 2>/dev/null || true
}
trap cleanup EXIT
sleep 0.5

passed=0
printf '\nTEST 1: Publisher litar på fel CA\n'
if mosquitto_pub -h localhost -p 8883 \
  --cafile generated/wrong-ca.crt \
  -t iot25/room-a/temperature \
  -m '{"value":21.7,"unit":"C"}' >/dev/null 2>&1; then
  echo "UNEXPECTED: TLS connection succeeded"
else
  echo "EXPECTED: TLS verification failed"
  passed=$((passed + 1))
fi

printf '\nTEST 2: Båda klienterna litar på rätt CA\n'
rm -f received.json
mosquitto_sub -h localhost -p 8883 \
  --cafile generated/ca.crt \
  -t iot25/room-a/temperature \
  -C 1 >received.json &
subscriber_pid=$!
sleep 0.2
mosquitto_pub -h localhost -p 8883 \
  --cafile generated/ca.crt \
  -t iot25/room-a/temperature \
  -m '{"sensorId":"room-a-temp-01","value":21.7,"unit":"C"}'
wait "$subscriber_pid"
subscriber_pid=""

if grep -q 'room-a-temp-01' received.json; then
  echo "RECEIVED: $(sed -n '1p' received.json)"
  passed=$((passed + 1))
else
  echo "UNEXPECTED: message was not received"
fi

printf '\nRESULT: %s/2 expected outcomes\n' "$passed"
test "$passed" -eq 2
