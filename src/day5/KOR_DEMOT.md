# C++-demo – MQTT till HTTP och konsument

Kör kommandona från denna mapp. C++17, CMake 3.24+, Python 3 samt Mosquitto broker, pub och sub behövs. Starta alla delar på samma dator. JSON är demots referensformat. Casets XML-grupper använder sina valideringsfunktioner från dag 4.

## Bygg

Öppna Developer PowerShell for Visual Studio i Windows, eller en terminal med GCC/Clang i Linux.

```text
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

C++-biblioteken cpp-httplib v0.51.0 och nlohmann/json v3.12.0 medföljer under `vendor/include`. CMake använder dem direkt och hämtar inget från internet. Licenser finns under `vendor/licenses`.

## Windows – fyra terminaler

Terminal 1, lokal broker:

```powershell
& 'C:\Program Files\mosquitto\mosquitto.exe' -c mosquitto.conf -v
```

Terminal 2, API:

```powershell
.\build\Debug\api.exe
```

Terminal 4, kontrollera tomt lager och skapa sensorfilen:

```powershell
.\build\Debug\consumer.exe
.\build\Debug\sensor.exe reading.json
```

Konsumenten visar HTTP 404 före första mätningen. Terminal 3, starta mottagaren:

```powershell
python capture.py 'C:\Program Files\mosquitto\mosquitto_sub.exe' received.json
```

Publicera inom 15 sekunder i terminal 4:

```powershell
& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1885 -t iot25/dag5/reading -f reading.json
```

När terminal 3 visar `Received ... bytes`, kör i terminal 4:

```powershell
.\build\Debug\bridge.exe received.json
.\build\Debug\consumer.exe
```

Förväntat: `HTTP 201` och sedan `temp-01: 21.7 C`. Starta mottagaren på nytt inför nästa publicering. `capture.py` sparar payloadens bytes utan att PowerShell ändrar kodningen eller lägger till terminaltext.

## Linux – samma flöde

Starta broker och API i var sin terminal:

```bash
mosquitto -c mosquitto.conf -v
```

```bash
./build/api
```

Generera filen i en klientterminal:

```bash
./build/consumer
./build/sensor reading.json
```

Starta mottagaren i en annan terminal:

```bash
python3 capture.py /usr/bin/mosquitto_sub received.json
```

Publicera inom 15 sekunder och invänta mottagarens avslut innan adaptern körs:

```bash
mosquitto_pub -h 127.0.0.1 -p 1885 -t iot25/dag5/reading -f reading.json
./build/bridge received.json
./build/consumer
```

## Feltest med Windows-kommandon

Se [testdatamappen](testdata/README.md) för färdiga scenarier, gränsvärden och förväntade resultat.

```powershell
.\build\Debug\bridge.exe testdata/invalid.json
curl.exe -i -H 'Content-Type: application/json' --data-binary '@testdata/invalid.json' http://127.0.0.1:8085/api/readings
curl.exe -i -H 'Content-Type: text/plain' --data-binary '@reading.json' http://127.0.0.1:8085/api/readings
curl.exe -i http://127.0.0.1:8085/api/readings/latest
```

I Linux används `./build/bridge` och `curl` med samma argument. Förväntat: adapterfel, HTTP 400, HTTP 415 och därefter det tidigare giltiga värdet. Stoppa API:t med Ctrl+C och kör adaptern igen för att visa ett transportfel. Efter omstart är lagret tomt tills en ny giltig mätning skickas.

## Automatiskt integrationstest

Stoppa manuella broker- och API-processer först. Testet kräver lediga portar 1885 och 8085 och stänger endast sina egna processer.

```powershell
python test_integration.py build/Debug 'C:\Program Files\mosquitto'
```

```bash
python3 test_integration.py build /usr/bin
```

Testet använder retained för ett deterministiskt MQTT-test utan kapplöpning vid prenumeration. Den manuella demon använder normalt ingen retained-publicering. Testet kontrollerar riktig MQTT-överföring, HTTP-status, validering, oförändrat värde vid fel, tjänsteavbrott och återhämtning.

## Avgränsningar

Detta är en engångsadapter med filöverlämning. API:t behåller ett enda värde i minnet och har ingen autentisering. HTTP och MQTT går okrypterat på loopback. En produktionslösning behöver bland annat åtkomstkontroll, TLS, ett beslut om lagring och en strategi för missade eller dubbla meddelanden. Fördjupningen följer senare i kursen.
