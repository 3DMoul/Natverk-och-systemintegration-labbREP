#!/usr/bin/env bash
set -euo pipefail

mkdir -p generated

openssl req -x509 -newkey rsa:2048 -nodes -days 2 \
  -subj "/CN=IOT25 Demo CA" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign" \
  -keyout generated/ca.key \
  -out generated/ca.crt >/dev/null 2>&1

openssl req -newkey rsa:2048 -nodes \
  -subj "/CN=localhost" \
  -keyout generated/server.key \
  -out generated/server.csr >/dev/null 2>&1

printf '%s\n' \
  'subjectAltName=DNS:localhost,IP:127.0.0.1' \
  'extendedKeyUsage=serverAuth' > generated/server.ext

openssl x509 -req -days 2 \
  -in generated/server.csr \
  -CA generated/ca.crt \
  -CAkey generated/ca.key \
  -CAcreateserial \
  -extfile generated/server.ext \
  -out generated/server.crt >/dev/null 2>&1

chmod 600 generated/ca.key generated/server.key
