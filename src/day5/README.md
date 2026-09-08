# Dag 5 – Elevpaket: MQTT, API och konsument

Paketet innehåller källkod till fem C++-program, byggkonfiguration, elva testdatafiler, automatiska tester, Mosquitto-konfiguration och C++-bibliotek med licenser. Packa upp hela zip-filen innan du börjar. Arbeta i den uppackade mappen `dag_5_demo`.

## Verktyg som behöver vara installerade

Verktygen och Mosquitto installeras separat. När de finns på datorn kan C++-programmen byggas utan internet. Python använder enbart standardbiblioteket, så ingen pip-installation behövs.

| Verktyg | Windows | Linux, Ubuntu/Debian |
|---|---|---|
| C++17-kompilator | Visual Studio eller Build Tools med Desktop development with C++ och Windows SDK | G++ från build-essential |
| CMake 3.24 eller senare | CMake i PATH | CMake |
| Python 3 | Python i PATH, anropas som python | Python3 |
| Mosquitto | Broker samt mosquitto_pub och mosquitto_sub, normalt i C:\Program Files\mosquitto | Mosquitto och mosquitto-clients |
| Curl | curl.exe | Curl |

Hämta verktygen från [Visual Studio](https://visualstudio.microsoft.com/downloads/), [CMake](https://cmake.org/download/), [Python](https://www.python.org/downloads/) och [Mosquitto](https://mosquitto.org/download/). Använd gärna klassens redan förberedda verktygsmiljö.

## 1. Kompilera

Programmen bildar följande kedja: sensorfil, MQTT-publicering, broker, MQTT-mottagare, mottagen fil, adapter, API och konsument. Filerna gör övergångarna synliga. Nätverksanropen går på datorns loopback-adress `127.0.0.1`. Port 1885 används av brokern och port 8085 av API:t.

| Källfil | Uppgift |
|---|---|
| sensor.cpp | Validera en intern mätning och skriva JSON till fil |
| bridge.cpp | Parsa mottagen fil, validera och göra HTTP POST |
| api.cpp | Validera inkommande mätningar, lagra senaste värdet och besvara GET |
| consumer.cpp | Hämta mätningen via HTTP GET och visa den |
| contract_test.cpp | Kontrollera datakontraktet utan nätverk |

`reading.hpp` innehåller den gemensamma datamodellen och funktioner för parsning, validering och serialisering. Python används för MQTT-fångst och teststöd, medan applikationslogiken ligger i C++.

I Windows: öppna Developer PowerShell for Visual Studio och gå till den uppackade mappen. I Linux: öppna en terminal i samma mapp.

```text
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Förväntat: fem byggda program och godkänt kontraktstest. I Windows med Visual Studio-generatorn finns programmen under `build/Debug`. I Linux med standardgeneratorn finns de under `build`.

## 2. Testa hela flödet automatiskt

Byggkommandona ovan har olika uppgifter:

| Kommando | Vad händer och varför? |
|---|---|
| cmake -S . -B build | Läser projektbeskrivningen i aktuell mapp och skapar byggfiler i build för din kompilator |
| cmake --build build --config Debug | Kompilerar C++ och länkar systembiblioteken till körbara program |
| ctest --test-dir build -C Debug --output-on-failure | Kör kontraktstestet så att datamodellens regler kontrolleras före nätverkstestet |

Stanna vid första felet och lös det innan du fortsätter. CTest visar ett registrerat test, men det testet innehåller flera datafall. Förväntat är att 100 procent av testerna passerar. Efter en ändring i en befintlig C++-fil kör du byggkommandot och testerna igen.

Stoppa egna tidigare demoprocesser på port 1885 och 8085. Testet startar broker och API, skickar MQTT-data, kör adaptern, läser API:t och testar fel. Det avslutar sina egna processer efteråt.

Windows:

```powershell
python test_integration.py build/Debug 'C:\Program Files\mosquitto'
```

Linux:

```bash
python3 test_integration.py build /usr/bin
```

Anpassa Mosquitto-sökvägen om programmen finns på annan plats. Förväntat: PASS för testfiler, MQTT-topic och integration, följt av normal avslutning. Ett fel ger ett felmeddelande eller AssertionError och en exitkod som inte är noll.

## 3. Kör själv steg för steg

### A. Förbered terminalerna

Öppna fyra terminaler i den uppackade mappen `dag_5_demo` och märk dem T1 Broker, T2 API, T3 Mottagare och T4 Klient. Kontrollera arbetskatalogen med `Get-Location` i PowerShell. Med `Get-ChildItem` ska du se exempelvis `capture.py`, `CMakeLists.txt` och mappen `build`. Relativa filnamn i kommandona räknas från denna katalog.

Kommandona nedan gäller Windows. Linux-kommandon finns i [KOR_DEMOT.md](KOR_DEMOT.md). Det automatiska testet ska vara avslutat innan du börjar, eftersom det använder samma portar.

### B. Starta brokern i T1

```powershell
& 'C:\Program Files\mosquitto\mosquitto.exe' -c mosquitto.conf -v
```

`-c` väljer konfigurationen som får brokern att lyssna på `127.0.0.1:1885`. `-v` visar loggar över anslutningar. Brokern förmedlar meddelanden efter topic men kontrollerar inte temperaturens enhet eller rimlighet. Låt den fortsätta köra. Om porten är upptagen behöver den tidigare egna demobrokern avslutas först.

### C. Starta API:t i T2 och kontrollera tomt lager från T4

I T2:

```powershell
.\build\Debug\api.exe
```

API:t lyssnar på `127.0.0.1:8085` och ska fortsätta köra. I T4:

```powershell
curl.exe -i http://127.0.0.1:8085/health
.\build\Debug\consumer.exe
```

Hälsokontrollen ger HTTP 200 och `ok`, eftersom tjänsten svarar. Konsumenten ger HTTP 404 och `no reading`, eftersom ingen mätning har skickats. Det är ett förväntat resultat. Ett transportfel betyder i stället att inget HTTP-svar mottogs, så kontrollera då att API:t körs i T2.

### D. Skapa mätningen i T4

```powershell
.\build\Debug\sensor.exe reading.json
Get-Content reading.json
```

Sensorn skapar den interna mätningen `temp-01`, 21,7 C, validerar den och serialiserar den till filen `reading.json`. Programmet skickar ännu inget på nätverket. Kontrollera fälten `sensorId`, `value` och `unit` innan du fortsätter.

### E. Starta mottagaren i T3 och publicera från T4

I T3:

```powershell
python capture.py 'C:\Program Files\mosquitto\mosquitto_sub.exe' received.json
```

Capture startar en subscriber som ansluter till brokern och prenumererar på `iot25/dag5/reading`. Den väntar på ett meddelande i högst 15 sekunder. Payload sparas i `received.json` utan terminalens kodningsomvandling. Filen töms vid start så att en gammal mätning inte kan misstas för ny data.

Publicera inom 15 sekunder i T4:

```powershell
& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1885 -t iot25/dag5/reading -f reading.json
```

`-h` anger brokeradress, `-p` port, `-t` topic och `-f` filen vars innehåll skickas. Publishern avslutas efter sändningen. Brokern förmedlar meddelandet till prenumeranten. Ingen retained-publicering används här, så mottagaren måste vara ansluten först.

Invänta `Received ... bytes` och återkommen prompt i T3. Om mottagaren får timeout, starta den igen och publicera på nytt. Fortsätt inte med en tom mottagen fil.

### F. Skicka till API:t och läs tillbaka i T4

```powershell
Get-Content received.json
.\build\Debug\bridge.exe received.json
.\build\Debug\consumer.exe
```

Adaptern parsar filen till en intern `Reading`, validerar den och skickar HTTP POST till `/api/readings`. API:t validerar igen eftersom det måste kontrollera indata från alla klienter. Vid framgång svarar det HTTP 201 och ersätter sin senaste mätning.

Konsumenten gör GET till `/api/readings/latest`, parsar svaret och skriver `temp-01: 21.7 C`. Jämför med ursprungsvärdet. Du har nu följt en mätning genom MQTT och HTTP. Konsumenten uppdaterar inte utskriften automatiskt utan måste köras igen för att läsa en ny mätning.

### G. Prova fel och förstå var de upptäcks

Med API:t igång och en giltig mätning lagrad, kör i T4:

```powershell
.\build\Debug\bridge.exe testdata/invalid.json
$LASTEXITCODE
.\build\Debug\consumer.exe
```

Filen har en sträng i `value` i stället för ett tal. Adaptern ska stoppa den före nätverksanropet och ge exitkod 1. Konsumenten ska fortfarande visa tidigare giltig mätning. Skicka därefter samma fil direkt till API:t:

```powershell
curl.exe -i -H 'Content-Type: application/json' --data-binary '@testdata/invalid.json' http://127.0.0.1:8085/api/readings
```

`--data-binary` skickar filens bytes, och `@` betyder att argumentet är ett filnamn. Headern anger dataformatet. API:t ska svara HTTP 400. Det visar varför mottagaren behöver egen validering även när den vanliga adaptern redan kontrollerar data.

Fortsätt med alla scenarier i [SCENARIER.md](SCENARIER.md). Där finns testfil, kommandon, terminal, förväntat resultat och återställning för syntaxfel, saknade fält, enhet, temperaturgränser, extra fält, fel header, stor body, fel MQTT-topic och API-avbrott. Curl med `-i` visar HTTP-status men ger inte automatiskt en fel-exitkod för HTTP 400.

### H. Avsluta

Stoppa broker i T1 och API i T2 med Ctrl+C. Avsluta eventuell väntande mottagare i T3. Övriga program avslutas efter varje operation. När API:t stängs försvinner mätningen eftersom den bara lagras i minnet. Efter nästa start är därför HTTP 404 förväntat tills en ny giltig mätning skickats.

## Material att följa under arbetet

* Läs [körinstruktionen](KOR_DEMOT.md) för de fyra terminalerna
* Följ [teori och scenarier](SCENARIER.md) för startordning, program, testfiler och förväntade svar
* Använd [testdataöversikten](testdata/README.md) för giltiga värden och felvarianter

Scenariomanuset har tidsangivelser för lärardemonstrationen. Vid eget arbete kan du följa samma ordning i din egen takt. Kommandona i det detaljerade scenariomanuset gäller Windows. Körinstruktionen innehåller även Linux-kommandon.

## Viktigt för testresultaten

* Starta API:t före adapter och konsument
* Starta MQTT-mottagaren före publicering och publicera inom 15 sekunder
* Vänta tills received.json har tagits emot innan adaptern körs
* Behåll invalid_syntax.json trasig eftersom det är ett avsiktligt parserfel
* Återställ normalvärdet med testdata/valid.json efter gränsvärdestester
* Förvänta HTTP 404 från ett nystartat API innan första giltiga mätningen

Demon körs lokalt på 127.0.0.1 och använder inte fysisk sensor. Nätverkskoden och valideringen ligger i C++, medan Python hjälper till med MQTT-fångst och tester. Grundflödet använder JSON och lagrar bara senaste värdet i minnet.

## Felsökning

| Problem | Kontroll |
|---|---|
| CMake hittar ingen kompilator | Installera C++-arbetsbelastningen och använd Developer PowerShell |
| Python eller CMake hittas inte | Kontrollera installation och PATH, öppna sedan en ny terminal |
| Mosquitto hittas inte | Kontrollera installationsmapp och att broker, pub och sub finns |
| Porten används redan | Avsluta den tidigare egna demoprocessen innan testet startas |
| Mottagaren får timeout | Starta den igen och publicera på rätt topic inom tidsgränsen |
| Fel vid körning inuti zip | Packa upp hela paketet och bygg från den uppackade mappen |

Källkodspaketet innehåller inga färdigbyggda program eller lokala mätningar från lärarens körning. `SHA256SUMS.txt` listar filernas kontrollsummor. C++-biblioteken finns i `vendor/include` och deras licenser i `vendor/licenses`.
