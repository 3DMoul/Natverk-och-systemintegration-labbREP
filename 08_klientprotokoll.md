# Klient- och felpolicyprotokoll – Dag 11

## API-kontrakt

| Fält | Värde |
|---|---|
| Metod och URL | |
| Lyckad status | |
| `Content-Type` | |
| Obligatoriska svarsfält | |
| Autentisering | |

## Budget

| Budget | Värde | Motivering |
|---|---:|---|
| Anslutningstimeout | | |
| Lästimeout | | |
| Maxförsök | | |
| Total tid | | |

## Felmatris

| Fel | Permanent/tillfälligt | Retry? | Väntan | Slutligt fel till användaren |
|---|---|---|---|---|
| 400 | | | | |
| 401/403 | | | | |
| 404 | | | | |
| 429 | | | | |
| 5xx | | | | |
| Timeout | | | | |
| Parse-/schemafel | | | | |

## Testresultat

| Scenario | Försök | Status/fel | Väntan | Validering | Resultat |
|---|---:|---|---:|---|---|
| `ok` | | | | | |
| `bad-request` | | | | | |
| `unauthorized` | | | | | |
| `rate-limit` | | | | | |
| `flaky` | | | | | |
| `bad-json` | | | | | |
| `wrong-type` | | | | | |
| `slow` | | | | | |

## Idempotens och loggning

**Varför är operationen säker eller osäker att återförsöka?**  

**Vilket ID skyddar mot dubletter?**  

**Vad loggas?**  

**Vad får inte loggas?**  
