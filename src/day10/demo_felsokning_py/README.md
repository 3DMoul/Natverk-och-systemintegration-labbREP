# Demo – Kontrollerad prestanda och felsökning i Python

Demon använder bara Python-standardbiblioteket och kör lokalt utan externa tjänster. C++-versionen har samma port, argument, scenarier och förväntade resultat.

## Terminalroller

* Terminal 1 representerar en lokal gateway/server och visar intern behandlingstid och loggar
* Terminal 2 representerar en simulerad sensorenhet och visar den svarstid klienten upplever
* Terminal 3 är valfri och representerar en passiv nätverksobservatör, inte en tjänst

Två terminaler behövs för att hålla klientens och serverns perspektiv synliga samtidigt.

## Kontroll

```bash
python -m unittest -v
```

## Grundläge

Terminal 1:

```bash
python server.py
```

Terminal 2:

```bash
python client.py --count 20 --interval-ms 50
curl http://127.0.0.1:8091/metrics
```

## Kontrollerad fördröjning

Stoppa endast servern i terminal 1 med `Ctrl+C` och starta:

```bash
python server.py --delay-ms 250
```

Kör samma klientkommando igen. Jämför klientens `latency_ms_*` med serverns `processing_ms`.

## Serverfel

```bash
python server.py --failure-every 3
```

Vart tredje anrop får HTTP 500 men har fortfarande nått servern.

## Valideringsfel

Kör normal server och därefter:

```bash
python client.py --count 9 --invalid-every 3
```

Vart tredje anrop får HTTP 400 eftersom `value` har fel typ.

## Anslutningsfel

```bash
python client.py --port 8092 --count 3
```

Ingen tjänst lyssnar normalt på port 8092. Klienten får därför ingen HTTP-status och servern får ingen loggrad.

## Valfri terminal 3

Linux:

```bash
sudo tcpdump -i lo -nn 'tcp port 8091'
```

Windows: Välj Npcap Loopback Adapter i Wireshark och använd displayfiltret `tcp.port == 8091`.

## Återställning

Stoppa servern och starta den utan parametrar. Kör fem lyckade anrop:

```bash
python server.py
python client.py --count 5
```

Server- och klientkommandot körs i varsin terminal.
