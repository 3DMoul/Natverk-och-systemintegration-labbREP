# Lokalt TLS-demo – Dag 7

Demot använder Python 3 och OpenSSL. All trafik stannar på `127.0.0.1`. Certifikat och privat nyckel skapas lokalt under `generated/`, som är undantagen från Git.

## Processroller

| Process | Representerad enhet | Ansvar |
|---|---|---|
| `https_server.py` | Lokal gateway eller tjänstevärd | Tar emot HTTPS och kontrollerar `X-API-Key` |
| `https_client.py` | Klient eller driftstation | Verifierar servercertifikat och begär senaste värdet |
| `run_demo.sh` | Lärarens styrning | Startar, testar och avslutar processerna i rätt ordning |

De två programmen körs som separata processer för att göra klientens och serverns kontroller synliga. `run_demo.sh` samlar dem i en terminal eftersom separata terminaler inte tillför något till grundflödet.

## Körning

```bash
bash run_demo.sh
```

Förväntat resultat:

* Test 1 visar `TLS_VERIFY_FAILED`
* Test 2 visar `HTTP 401`
* Test 3 visar `HTTP 200` och ett JSON-värde
* Summeringen visar `3/3 expected outcomes`

## Filer

* `generate_certificates.sh` skapar en lokal CA och ett servercertifikat för `localhost` och `127.0.0.1`
* `https_server.py` kör HTTPS-tjänsten
* `https_client.py` genomför ett anrop och skriver vilket säkerhetslager som godkände eller stoppade det
* `run_demo.sh` utför de tre scenarierna och städar serverprocessen

Certifikaten är en labbidentitet. Återanvänd dem inte i andra miljöer.
