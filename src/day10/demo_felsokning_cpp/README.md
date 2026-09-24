# Demo – Kontrollerad prestanda och felsökning i C++

Detta är C++17-versionen av Python-demot. Båda versionerna använder port 8091, samma argument, endpoints, felmoder, `request_id` och sammanfattande mätetal.

## Terminalroller

* Terminal 1 representerar en lokal gateway/server och kör `troubleshooting_server`
* Terminal 2 representerar en simulerad sensorenhet och kör `troubleshooting_client`
* Terminal 3 är en valfri passiv nätverksobservatör, inte en tjänst

## Bygg

Linux, macOS eller MinGW:

```bash
cmake -S . -B build
cmake --build build
```

Visual Studio Developer PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

## Grundläge

Terminal 1:

```bash
./build/troubleshooting_server
```

Terminal 2:

```bash
./build/troubleshooting_client --count 20 --interval-ms 50
curl http://127.0.0.1:8091/metrics
```

I Visual Studio finns programmen normalt under `build\Debug` och har ändelsen `.exe`.

## Kontrollerad fördröjning och serverfel

```bash
./build/troubleshooting_server --delay-ms 250
./build/troubleshooting_server --failure-every 3
```

Kör bara ett serverkommando åt gången. Klientkommandot är oförändrat.

## Valideringsfel och anslutningsfel

```bash
./build/troubleshooting_client --count 9 --invalid-every 3
./build/troubleshooting_client --port 8092 --count 3
```

## Återställning

Stoppa servern med `Ctrl+C`, starta den utan felargument och kör fem anrop:

```bash
./build/troubleshooting_server
./build/troubleshooting_client --count 5
```
