#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if test "$#" -ne 1; then
  echo "Usage: $0 LAPTOP_LAN_IP"
  echo "Example: $0 192.168.1.42"
  exit 2
fi

laptop_ip=$1
if ! printf '%s\n' "$laptop_ip" | grep -Eq '^([0-9]{1,3}\.){3}[0-9]{1,3}$'; then
  echo "ERROR: expected an IPv4 address, got: $laptop_ip"
  exit 2
fi

for command_name in openssl mosquitto mosquitto_passwd; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "MISSING: $command_name"
    exit 2
  fi
done

mkdir -p generated ../main/certs

openssl req -x509 -newkey rsa:2048 -nodes -days 30 \
  -subj "/CN=IOT25 Day 8 Lab CA" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign" \
  -keyout generated/ca.key \
  -out generated/ca.crt

openssl req -newkey rsa:2048 -nodes \
  -subj "/CN=$laptop_ip" \
  -keyout generated/server.key \
  -out generated/server.csr

printf '%s\n' \
  "subjectAltName=IP:$laptop_ip,IP:127.0.0.1,DNS:localhost" \
  'extendedKeyUsage=serverAuth' >generated/server.ext

openssl x509 -req -days 30 \
  -in generated/server.csr \
  -CA generated/ca.crt \
  -CAkey generated/ca.key \
  -CAcreateserial \
  -extfile generated/server.ext \
  -out generated/server.crt

chmod 600 generated/ca.key generated/server.key
cp generated/ca.crt ../main/certs/laptop-ca.crt

echo
echo "Create the MQTT user 'day8'. Choose a lab password and give it to the students."
mosquitto_passwd -c generated/passwords day8

echo
echo "READY"
echo "Broker URI: mqtts://$laptop_ip:8883"
echo "Username: day8"
echo "Public CA copied to: ../main/certs/laptop-ca.crt"
echo "Private keys remain in: generated/"
