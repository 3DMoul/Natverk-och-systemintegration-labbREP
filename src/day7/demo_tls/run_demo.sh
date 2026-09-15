#!/usr/bin/env bash
set -u

cd "$(dirname "$0")" || exit 1
bash generate_certificates.sh || exit 1

demo_key="iot25-local-demo-key"
IOT25_API_KEY="$demo_key" python3 https_server.py >server.log 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true; wait "$server_pid" 2>/dev/null || true' EXIT

for attempt in 1 2 3 4 5; do
  if python3 -c 'import socket; s=socket.create_connection(("127.0.0.1",8443),0.2); s.close()' 2>/dev/null; then
    break
  fi
  sleep 0.2
done

passed=0

printf '\nTEST 1: Klienten saknar betrodd CA\n'
python3 https_client.py
test "$?" -eq 20 && passed=$((passed + 1))

printf '\nTEST 2: TLS godkänns men API-nyckeln saknas\n'
python3 https_client.py --trust-ca
test "$?" -eq 10 && passed=$((passed + 1))

printf '\nTEST 3: TLS och API-nyckel godkänns\n'
python3 https_client.py --trust-ca --api-key "$demo_key"
test "$?" -eq 0 && passed=$((passed + 1))

printf '\nRESULT: %s/3 expected outcomes\n' "$passed"
test "$passed" -eq 3
