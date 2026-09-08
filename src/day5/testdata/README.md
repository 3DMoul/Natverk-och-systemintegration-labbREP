# Testdata för lärardemonstrationen

Kör kommandona från `dag_5/demo_cpp`. Starta API:t först. Filerna testas automatiskt av `test_integration.py` mot både C++-adaptern och API:t.

| Fil | Scenario | Förväntat resultat |
|---|---|---|
| [Valid](valid.json) | Giltig temperatur 21,7 C | Adapter lyckas, API svarar 201 |
| [Invalid](invalid.json) | Value är strängen warm | Adapter stoppar, API svarar 400 |
| [Invalid syntax](invalid_syntax.json) | Avsiktligt saknad avslutande klammer | Parserfel, API svarar 400 |
| [Missing sensor ID](missing_sensor_id.json) | Obligatoriskt sensorId saknas | Strukturfel, API svarar 400 |
| [Empty sensor ID](empty_sensor_id.json) | SensorId är tomt | Valideringsfel, API svarar 400 |
| [Wrong unit](wrong_unit.json) | Enheten är kg | Valideringsfel, API svarar 400 |
| [Below range](below_range.json) | −50,1 C | Valideringsfel, API svarar 400 |
| [Above range](above_range.json) | 100,1 C | Valideringsfel, API svarar 400 |
| [Lower boundary](lower_boundary.json) | Exakt −50 C | Godkänt gränsvärde, API svarar 201 |
| [Upper boundary](upper_boundary.json) | Exakt 100 C | Godkänt gränsvärde, API svarar 201 |
| [Extra field](extra_field.json) | Extra fält room | API svarar 201, room ignoreras och förs inte vidare |

## Demonstrera ett fel

Skicka först en giltig mätning. Skicka sedan ett fel genom adaptern och direkt till API:t. Kontrollera att den giltiga mätningen ligger kvar:

```powershell
.\build\Debug\bridge.exe testdata/valid.json
.\build\Debug\bridge.exe testdata/wrong_unit.json
curl.exe -i -H 'Content-Type: application/json' --data-binary '@testdata/wrong_unit.json' http://127.0.0.1:8085/api/readings
.\build\Debug\consumer.exe
```

Adaptern returnerar exitkod 1 vid datafel utan att anropa API:t. Direkt POST av samma felaktiga data ger HTTP 400 med fältet `error`. Konsumenten ska fortfarande visa `temp-01: 21.7 C`.

## Scenarier som beror på miljö eller anrop

| Scenario | Indata eller åtgärd | Förväntat |
|---|---|---|
| Fel Content-Type | Giltig JSON med header text/plain | HTTP 415 |
| För stor body | 4097 byte, genereras i integrationstestet | HTTP 413 |
| API avstängt | Stoppa API:t och kör adapter med valid.json | Transportfel och exitkod 1 |
| API omstartat | Starta om och kör konsument före ny POST | HTTP 404 eftersom minneslagret är tomt |
| Fel MQTT-topic | Publicera till iot25/dag5/other | Ingen payload till prenumerationen på iot25/dag5/reading |

Återställ normalvärdet med `valid.json` efter gränsvärdestesterna. NaN testas som ett internt C++-värde i kontraktstestet eftersom NaN inte är ett giltigt JSON-tal. Filen `invalid_syntax.json` ska vara syntaktiskt trasig och ska inte rättas av editorn.
