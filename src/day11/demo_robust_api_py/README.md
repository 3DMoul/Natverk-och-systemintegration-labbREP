# Demo – Robust API-klient i Python

Python-versionen använder endast standardbiblioteket. C++-versionen har samma port, scenarier och beslut.

## Terminalroller

* Terminal 1 representerar en extern API-tjänst och kör `server.py`
* Terminal 2 representerar IoT-gatewayens klient och kör `client.py`

## Kör

Terminal 1:

```bash
python server.py
```

Terminal 2:

```bash
python client.py ok
python client.py bad-request
python client.py unauthorized
python client.py rate-limit
python client.py flaky
python client.py bad-json
python client.py wrong-type
python client.py slow
```

Scenarier som slutar med ett avsiktligt fel returnerar exit code 1.

## Test

```bash
python -m unittest -v
```

Stoppa servern med `Ctrl+C`.
