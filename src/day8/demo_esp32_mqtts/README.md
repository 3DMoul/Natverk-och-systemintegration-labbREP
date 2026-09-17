# ESP32-C6 MQTTS-demo – Dag 8

Projektet är skrivet för ESP-IDF 6.0.x. Det skapar två periodiska `esp_timer`, fyra egna FreeRTOS-tasks, räknare och eventhantering för Wi-Fi och MQTT. Små sensorevent går genom en FreeRTOS-kö och publiceras över MQTTS. ESP-MQTT deklareras som separat komponent eftersom det flyttades ut ur ESP-IDF från version 6.0.

## Applikationens tasks

* `temperature_task`: Väcks av temperaturtimern och producerar ett temperaturevent
* `humidity_task`: Väcks av fukttimern och producerar ett fuktevent
* `publish_task`: Konsumerar kön och är ensam ansvarig för MQTT-publicering
* `statistics_task`: Skriver kö- och leveransräknare var tionde sekund

ESP-IDF:s Wi-Fi- och MQTT-komponenter använder dessutom interna tasks. De fyra ovan är de körflöden som kurskoden själv skapar och som studenterna ska följa.

## Systemroller och terminaler

* Terminal 1 – Lärarlaptop som lokal gateway: Kör Mosquitto-brokern och visar anslutningar
* Terminal 2 – ESP32-C6 som sensorenhet: Bygger, flashar och visar seriell logg från timers, kö, Wi-Fi och MQTT
* Terminal 3 – Lärarlaptop som konsument: Visar meddelanden som faktiskt nått brokern

Tre terminaler används eftersom brokerlogg, enhetens interna logg och konsumentens mottagningsbevis är tre olika perspektiv. Terminal 1 och 3 representerar två logiska tjänster på samma fysiska laptop/gateway. Om terminalbytena blir röriga kan brokerloggen skrivas till fil och endast terminal 2 och 3 visas.

## Säkerhetsmodell

Projektet använder `mqtts://`, användarnamn/lösenord och en privat lokal CA för att verifiera brokern på lärarlaptopen. Det offentliga CA-certifikatet byggs in i firmware. CA-nyckeln och brokerns privata servernyckel ligger endast på laptopen och får aldrig kopieras till ESP32-C6. Se [certifikatguiden för MCU](../../dag_7/09_certifikat_pa_mcu.md).

## Starta laptopbrokern först

Följ [guiden för laptopbrokern](laptop_broker/README.md). Den skapar ett servercertifikat för laptopens aktuella LAN-IP, ett tillfälligt labbkonto och `main/certs/laptop-ca.crt` som firmwarebygget behöver.

## Förbered konfiguration

Den gemensamma Wi-Fi- och MQTT-konfigurationen ligger i `sdkconfig.defaults` så att samma grundinställningar kan delas med studenterna. Kontrollera särskilt laptopens aktuella IP-adress innan utdelning. Generera därefter en lokal `sdkconfig` genom att aktivera ESP-IDF-miljön och köra från projektmappen:

```bash
idf.py set-target esp32c6
idf.py reconfigure
```

`sdkconfig.defaults` innehåller kursnätets delade labbuppgifter, laptopens MQTTS-URI, labbkontot och topic-prefixet. Broker-IP i URI:n måste vara samma IP som användes när certifikatet skapades. En grupp kan vid behov öppna `idf.py menuconfig` och ge sin lokala `sdkconfig` ett unikt topic-prefix, exempelvis `iot25/day8/grupp-04`.

`sdkconfig` genereras lokalt och är undantagen från Git. Observera att `sdkconfig.defaults` nu avsiktligt innehåller delade labbuppgifter i klartext. Använd aldrig ett personligt lösenord där och byt eller ta bort uppgifterna innan materialet sprids utanför kursgruppen.

## Bygg med kort sökväg vid behov

Vanligt bygge:

```powershell
idf.py build
```

I den här kursmappens Linux-sökväg kan ESP-IDF:s RISC-V-toolchain misstolka tecknet `ä`. Använd då en byggkatalog utan specialtecken:

```bash
idf.py -B /tmp/iot25_day8_build build
idf.py -B /tmp/iot25_day8_build -p /dev/ttyACM0 flash monitor
```

Om Windows ger för lång sökväg:

```powershell
idf.py -B C:\esp\proj\iot25_day8_build build
```

Samma `-B`-argument ska användas vid flashning och monitorering.

## Flashning och monitor

Identifiera vilken port din ESP32 är kopplad till och använd aktuell port:

```powershell
idf.py -p COMX flash monitor
```

Med kort byggsökväg:

```powershell
idf.py -B C:\esp\proj\iot25_day8_build -p COMX flash monitor
```

Avsluta monitor med `Ctrl+]`.

## Subscriber på lärarlaptopen

Öppna terminal 3 och prenumerera med en klient som litar på labbets CA. Ersätt IP och lösenord:

```bash
mosquitto_sub -h 192.168.1.42 -p 8883 \
  --cafile laptop_broker/generated/ca.crt \
  -u day8 -P 'LAB_PASSWORD' \
  -t 'iot25/day8/#' -v
```

Begränsa topic till dagens prefix. Lösenordet är ett tillfälligt labblösenord; använd inte ett personligt lösenord i kommandoraden.

## Förväntad seriell logg

* `WIFI_CONNECTED`
* `MQTT_CONNECTED laptop broker verified with embedded lab CA`
* Fyra `TASK_CREATED` med namn och prioritet
* Två `TIMER_STARTED` med namn och intervall
* `SENSOR` med producerande tasknamn och kölängd
* `PUBLISH` för temperatur var femte sekund
* `PUBLISH` för luftfuktighet var sjunde sekund
* `COUNTERS` var tionde sekund

Stoppa och starta laptopbrokern för ett kontrollerat avbrott. Det påverkar endast labbmiljön. Ta bort den tillfälliga brandväggsregeln efter passet.

Efter återanslutning ska både ett nytt `MQTT_CONNECTED` och ett nytt mottaget meddelande observeras.

## Offlinepolicy

Referensprojektet tappar event som publiceringsuppgiften tar emot när MQTT är frånkopplat och ökar `offline_dropped`. Om kön är full ökar `queue_dropped`. Detta gör dataförlusten synlig. Det är en undervisningspolicy, inte ett generellt produktionskrav.
