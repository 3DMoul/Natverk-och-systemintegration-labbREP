# Demo – Strukturerade loggar, mätetal och dashboard i C++

Detta är C++17-varianten av Python-demot. De två varianterna har samma terminalroller, fyra klientscenarier, HTTP-endpoints, `X-Request-ID`, logghändelser, mätetal och dashboard. Välj ett språkspår; laborationens resultat är detsamma.

Koden använder endast operativsystemets sockets och C++ standardbibliotek. På Windows länkar CMake automatiskt mot Winsock.

## Vad komponenterna representerar

* Terminal 1 – `observability_server` representerar en lokal gateway/API
* Terminal 2 – `observability_client` representerar en sensorenhet
* Terminal 3, valfri – Wireshark eller tcpdump representerar en passiv observatör, inte en extra tjänst

## Bygg på Linux, macOS eller med MinGW

```bash
cd dag_9/demo_observability_cpp
cmake -S . -B build
cmake --build build
```

## Bygg i Visual Studio Developer PowerShell

```powershell
cd dag_9\demo_observability_cpp
cmake -S . -B build
cmake --build build --config Debug
```

## Kör

Terminal 1 på Linux/macOS/MinGW:

```bash
./build/observability_server
```

Terminal 2:

```bash
./build/observability_client
curl http://127.0.0.1:8090/api/metrics
curl http://127.0.0.1:8090/metrics
```

Med Visual Studio används i stället:

```powershell
.\build\Debug\observability_server.exe
.\build\Debug\observability_client.exe
curl.exe http://127.0.0.1:8090/api/metrics
```

Öppna `http://127.0.0.1:8090/dashboard` i webbläsaren.

Annan port anges på båda programmen:

```bash
./build/observability_server --port 8091
./build/observability_client --port 8091
```

## Förväntade scenarier

| `request_id` | Begäran | Resultat |
|---|---|---|
| `demo-temp-1` | Giltig temperatur | 202 Accepted |
| `demo-humidity-1` | Giltig luftfuktighet | 202 Accepted |
| `demo-invalid-1` | `value` är text | 400 Bad Request |
| `demo-missing-1` | Okänd resurs | 404 Not Found |

Stoppa servern med `Ctrl+C`. Programmet använder ingen databas och skapar inga datafiler.
