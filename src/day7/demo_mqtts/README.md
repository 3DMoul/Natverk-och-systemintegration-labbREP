# Lokalt MQTTS-demo – Dag 7

Demot kör en lokal Mosquitto-broker med TLS på port 8883. En lokal CA signerar brokerns servercertifikat. En publisher och subscriber ansluter med CA-certifikatet och använder MQTT över TLS.

## Processroller

| Process | Representerad enhet | Ansvar |
|---|---|---|
| `mosquitto` | Lokal gateway eller broker-server | Visar servercertifikatet och förmedlar MQTT-meddelandet |
| `mosquitto_pub` | ESP32-C6 eller sensorsimulator | Verifierar brokern och publicerar telemetri |
| `mosquitto_sub` | Bearbetningstjänst eller konsument | Verifierar brokern och tar emot telemetri |
| `run_demo.sh` | Lärarens styrning | Skapar certifikat och kör processerna i rätt ordning |

Publisher och subscriber representerar separata klientenheter även om båda körs på lärardatorn. Brokerprocessen representerar en tredje enhet. Grunddemot samlar loggarna i en terminal för att minska handgreppen.

## Förutsättningar

* Mosquitto broker
* `mosquitto_pub` och `mosquitto_sub`
* OpenSSL
* Bash

## Körning

```bash
bash run_demo.sh
```

Förväntat:

* Testet med fel CA misslyckas under TLS-verifiering
* Testet med rätt CA levererar ett JSON-meddelande på `iot25/room-a/temperature`
* Slutraden visar `2/2 expected outcomes`

Certifikaten skapas i `generated/` och gäller endast för denna lokala labb. Brokerns privata nyckel ska stanna på broker-enheten. En ESP32-klient behöver CA-certifikatet, inte brokerns privata nyckel.
