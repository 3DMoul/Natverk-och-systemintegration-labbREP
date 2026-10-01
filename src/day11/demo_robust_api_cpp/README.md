# Demo – Robust API-klient i C++

C++17-versionen använder operativsystemets sockets och standardbiblioteket. Den har samma port, scenarier och beslut som Python-versionen.

## Terminalroller

* Terminal 1 representerar en extern API-tjänst och kör `robust_api_server`
* Terminal 2 representerar IoT-gatewayens klient och kör `robust_api_client`

## Bygg

```bash
cmake -S . -B build
cmake --build build
```

Visual Studio-användare lägger till `--config Debug` vid byggkommandot och kör programmen under `build\Debug`.

## Kör

Terminal 1:

```bash
./build/robust_api_server
```

Terminal 2:

```bash
./build/robust_api_client ok
./build/robust_api_client bad-request
./build/robust_api_client unauthorized
./build/robust_api_client rate-limit
./build/robust_api_client flaky
./build/robust_api_client bad-json
./build/robust_api_client wrong-type
./build/robust_api_client slow
```

Scenarier som slutar med avsiktligt fel returnerar exit code 1. Stoppa servern med `Ctrl+C`.
