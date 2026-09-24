# Demo – Strukturerade loggar, mätetal och dashboard i Python

Detta är Python-varianten av demon. Den har samma scenarier, endpoints och resultat som [C++-varianten](../demo_observability_cpp/README.md). Demon använder endast Pythons standardbibliotek. Den kör lokalt och skickar ingen data till internet.

## Vad komponenterna representerar

* Terminal 1 – `server.py` representerar en lokal gateway/API. Den validerar mätningar, loggar händelser och exponerar mätetal.
* Terminal 2 – `client.py` representerar en sensorenhet. Den skickar fyra reproducerbara scenarier med kända korrelations-ID:n.
* Terminal 3, valfri – Wireshark eller tcpdump representerar en passiv observatör på datorn, inte en extra tjänst.

Två terminaler behövs för att klientens och serverns perspektiv ska synas samtidigt. Den tredje används bara när nätverkstrafiken ska jämföras med applikationsloggarna.

## Kör

Terminal 1:

```bash
cd dag_9/demo_observability_py
python3 server.py
```

Terminal 2:

```bash
cd dag_9/demo_observability_py
python3 client.py
curl http://127.0.0.1:8090/api/metrics
curl http://127.0.0.1:8090/metrics
```

Öppna dashboarden på `http://127.0.0.1:8090/dashboard`.

Server och klient kan använda en annan port:

```bash
python3 server.py --port 8091
python3 client.py --port 8091
```

## Scenarier

| `request_id` | Begäran | Förväntat resultat |
|---|---|---|
| `demo-temp-1` | Giltig temperatur | 202 Accepted |
| `demo-humidity-1` | Giltig luftfuktighet | 202 Accepted |
| `demo-invalid-1` | `value` är text | 400 Bad Request |
| `demo-missing-1` | Okänd resurs | 404 Not Found |

## Endpoints

| Metod och sökväg | Syfte |
|---|---|
| `POST /api/readings` | Validera och acceptera ett mätvärde |
| `GET /health` | Kontrollera att processen svarar |
| `GET /api/metrics` | Mätetal som JSON för dashboarden |
| `GET /metrics` | Samma centrala mätetal som enkel text |
| `GET /dashboard` | Lokal HTML-dashboard |

## Automatisk kontroll

Kontrollen startar servern på en tillfällig ledig port, kör scenarierna och verifierar räknare och korrelations-ID:

```bash
python3 test_demo.py
```

## Stoppa

Tryck `Ctrl+C` i serverterminalen. Servern behöver ingen databas och lämnar inga genererade filer efter sig.
