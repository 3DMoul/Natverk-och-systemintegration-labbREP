# Teori och lärardemo – Dag 5

## 1. Återkoppling till dag 4

En intern `Reading` är programmets data. JSON och XML är representationer som kan färdas mellan program. Sändaren validerar före serialisering och mottagaren parsar och validerar innan värdet används. Ett bibliotek kontrollerar syntax, medan vår kod kontrollerar fält, enhet och rimlighet.

## 2. Tjänsteorienterad arkitektur

En tjänst erbjuder en avgränsad förmåga genom ett beskrivet gränssnitt. I Smart inomhusmiljö kan en tjänst ta emot mätningar och erbjuda den senaste mätningen. Klienten behöver känna till kontraktet men inte hur tjänsten lagrar datan.

Tjänsteorienterad arkitektur, SOA, gör ansvar och gränssnitt centrala. Det underlättar återanvändning och integration med andra system. Kostnaden är fler gränssnitt, versioner och felställen att hantera. SOA innebär inte att varje funktion måste bli en egen process eller mikrotjänst.

REST och SOAP från dag 3 kan båda användas mellan tjänster. Ett befintligt fastighetssystem kan ha ett SOAP-gränssnitt, medan en ny app använder ett REST-API. En adapter översätter då operationer och data enligt båda kontrakten. Dagens körbara adapter överbryggar MQTT och HTTP.

## 3. Distribuerade system och koppling

Komponenter kommunicerar över nätverksgränssnitt och kan ha olika livscykler. På en enda dator kan vi öva detta med separata processer. I drift kan sensorn finnas i ett rum, brokern på en gateway och API:t i ett datacenter.

| Mönster | I dagens IoT-system | Möjlighet | Begränsning |
|---|---|---|---|
| Request/response | Konsumenten gör GET | Direkt svar på en fråga | Servern måste vara nåbar |
| Publish/subscribe | Sensorn publicerar till broker | Flera mottagare utan mottagarlista i sensorn | Leverans beror på QoS, session och tillgänglighet |
| Adapter | MQTT-data skickas till HTTP-API | Olika gränssnitt kan samverka | Ett nytt felställe och kontrakt att underhålla |

Lös koppling betyder att färre interna detaljer delas. Den tar inte bort beroendet av ett gemensamt datakontrakt. En ändring från `value` till `temperature` kan fortfarande bryta integrationen.

## 4. Flöde och ansvar

```text
C++-sensor → JSON-fil → MQTT-publisher → Broker TCP 1885
                                           ↓
                          MQTT-subscriber → mottagen fil
                                           ↓
                                      C++-adapter
                                           ↓ POST
                                  C++-API TCP 8085
                                           ↑ GET
                                     C++-konsument
```

Filerna är synliga överlämningar i engångsdemon. MQTT-trafiken och HTTP-trafiken går över riktiga sockets. En fortlöpande adapter skulle i stället använda en MQTT-klient och hantera inkommande meddelanden direkt. Den extra komplexiteten behövs inte för att visa dagens integrationsgränser.

## 5. Fel och dataansvar

Ett partiellt fel innebär exempelvis att brokern fungerar samtidigt som API:t är avstängt. Vi behöver testa varje sträcka. Timeout begränsar hur länge ett anrop väntar. Den garanterar inte att servern aldrig utförde operationen. Automatisk omsändning av POST kan därför skapa dubbletter i ett API som sparar historik.

Demon behåller endast senaste godkända mätning i minnet. Ogiltig data ska inte ersätta den. Efter omstart är lagret tomt. En gammal mätning kan fortfarande gå att läsa även när sensorn har stannat. Diskutera tidsstämpel och källans livstecken, men skilj dem från nätverkets nåbarhet.

## 6. Demonstrationsmanus 10:15–12:00

### Förberedelser och terminaler

Kommandona nedan gäller Windows PowerShell. Öppna fyra terminaler med arbetskatalogen `dag_5_demo`. Om terminalen står i kursens rotmapp, kör först:

```powershell
Set-Location dag_5_demo
```

Använd Python 3 som kommandot `python`. Mosquitto förutsätts finnas under `C:\Program Files\mosquitto`. Bygginstruktioner och Linux-motsvarigheter finns i [demots README](KOR_DEMOT.md). Bygg först programmen enligt README. Windowsprogrammen skapas i `build/Debug`.

| Terminal | App | Livslängd och ansvar |
|---|---|---|
| T1 – Broker | `mosquitto.exe` | Kör under MQTT-scenarierna och stoppas med Ctrl+C vid avslut |
| T2 – API | `api.exe` | Kör kontinuerligt utom vid det avsiktliga API-avbrottet |
| T3 – Mottagare | `python capture.py`, som startar `mosquitto_sub.exe` | Körs på nytt inför varje MQTT-publicering och avslutas efter ett meddelande eller timeout |
| T4 – Klient | `sensor.exe`, `mosquitto_pub.exe`, `bridge.exe`, `consumer.exe` och `curl.exe` | Varje kommando kör en operation och avslutas |

`sensor.exe` skapar en fil men publicerar inte till MQTT. `bridge.exe` läser en fil och anropar API:t men prenumererar inte själv på MQTT. `consumer.exe` läser API:t en gång och uppdaterar inte sin utskrift automatiskt.

Starta med en ny API-process och en ny lokal brokerprocess. Stoppa tidigare egna demoprocesser med Ctrl+C först. Då är API:ts minneslager tomt och tidigare retained-meddelanden från en manuell körning finns inte kvar i denna broker, vars konfiguration har `persistence false`. Publicera utan `-r` i det manuella manuset.

### A. Kod och tomt API, 10:15–10:25

Visa [reading.hpp](reading.hpp). Peka ut intern datamodell, validering, parsning och serialisering. I T4:

```powershell
.\build\Debug\contract_test.exe
```

Förväntat: `Contract failures: 0`. Här testas även ogiltiga interna värden före serialisering, inklusive NaN. NaN har ingen testdatafil eftersom det inte är ett giltigt JSON-tal.

Starta brokern i T1:

```powershell
& 'C:\Program Files\mosquitto\mosquitto.exe' -c .\mosquitto.conf -v
```

Starta API:t i T2 och låt det fortsätta köra:

```powershell
.\build\Debug\api.exe
```

I T4, innan någon adapter eller POST har körts:

```powershell
curl.exe -i http://127.0.0.1:8085/health
.\build\Debug\consumer.exe
```

Förväntat: `/health` ger HTTP 200 och `ok`, men konsumenten visar HTTP 404 med `no reading` och avslutas med exitkod 1. Tjänsten svarar men saknar mätning. Körs detta efter en tidigare lyckad POST blir resultatet i stället den lagrade mätningen, så startordningen är viktig.

### B. Giltig mätning genom hela kedjan, 10:25–10:40

T1 och T2 fortsätter köra. Generera sensorfilen i T4:

```powershell
.\build\Debug\sensor.exe reading.json
Get-Content reading.json
```

Förväntat innehåll: `sensorId=temp-01`, `value=21.7`, `unit=C`. Filen motsvarar [testdata/valid.json](testdata/valid.json), även om indrag och fältordning kan skilja sig.

Starta mottagaren i T3:

```powershell
python capture.py 'C:\Program Files\mosquitto\mosquitto_sub.exe' received.json
```

När mottagaren har startat, publicera inom 15 sekunder i T4:

```powershell
& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1885 -t iot25/dag5/reading -f reading.json
```

Invänta `Received ... bytes` och återkommen prompt i T3. Kör först därefter i T4:

```powershell
Get-Content received.json
.\build\Debug\bridge.exe received.json
.\build\Debug\consumer.exe
```

Förväntat: adaptern visar `HTTP 201` och konsumenten `temp-01: 21.7 C`. Följ fälten genom filerna och API-svaret. Om mottagaren fick timeout ska den startas om före en ny publicering. Kör inte adaptern med en tom `received.json`.

### C. Datakontraktets felscenarier, 10:40–11:00

T2 måste vara igång. T3 ska ha avslutats. Dessa scenarier använder testfiler direkt för att isolera valideringen från MQTT. T1 kan fortsätta köra men används inte av kommandona.

Sätt först en känd baslinje i T4:

```powershell
.\build\Debug\bridge.exe testdata/valid.json
.\build\Debug\consumer.exe
```

Gå igenom följande filer i ordning. Ändra variabeln `$scenarioFile` till filen för den aktuella raden och kör hela kommandoblocket under tabellen för varje scenario.

| Scenario | Värde för `$scenarioFile` | Vad som är fel | Förväntad felkategori |
|---|---|---|---|
| Fel datatyp | `testdata/invalid.json` | Value är strängen warm | Fält-/typkontroll |
| Trasig syntax | `testdata/invalid_syntax.json` | Avslutande klammer saknas | JSON-parserfel |
| Saknat fält | `testdata/missing_sensor_id.json` | SensorId saknas | Fält-/typkontroll |
| Tom identitet | `testdata/empty_sensor_id.json` | SensorId är tom sträng | Validering av sensorId |
| Fel enhet | `testdata/wrong_unit.json` | Unit är kg | Validering av enhet |
| Under intervallet | `testdata/below_range.json` | Value är −50,1 | Validering av temperatur |
| Över intervallet | `testdata/above_range.json` | Value är 100,1 | Validering av temperatur |

Kör i T4, här med typfelet som exempel:

```powershell
$scenarioFile = 'testdata/invalid.json'
Get-Content $scenarioFile
.\build\Debug\bridge.exe $scenarioFile
$LASTEXITCODE
.\build\Debug\consumer.exe
curl.exe -i -H 'Content-Type: application/json' --data-binary "@$scenarioFile" http://127.0.0.1:8085/api/readings
.\build\Debug\consumer.exe
```

För varje rad ska adaptern ge ett begripligt fel och exitkod 1 utan att göra en POST. Första konsumentkörningen visar att baslinjen finns kvar. Curl skickar samma fil direkt till API:t och ska få HTTP 400 med JSON-fältet `error`. Den andra konsumentkörningen ska fortfarande visa `temp-01: 21.7 C`.

Adapterns fält-/typfel har samma gemensamma feltext för saknat fält och fel typ. Parserfelets exakta rad-/kolumntext kan variera. Bedöm felkategori och oförändrad lagring. `curl.exe -i` visar HTTP-status men ger inte automatiskt en misslyckad process-exitkod för HTTP 400.

### D. Godkända gränsvärden och extra fält, 11:00–11:10

API:t i T2 fortsätter köra. Använd följande filer i T4:

| Scenario | Fil | Förväntat värde hos konsumenten |
|---|---|---|
| Nedre gräns ingår | `testdata/lower_boundary.json` | −50 C |
| Övre gräns ingår | `testdata/upper_boundary.json` | 100 C |
| Okänt extra fält | `testdata/extra_field.json` | 21,7 C, utan fältet room i API-svaret |

Kör för varje fil, med nedre gränsen som exempel:

```powershell
$scenarioFile = 'testdata/lower_boundary.json'
Get-Content $scenarioFile
.\build\Debug\bridge.exe $scenarioFile
.\build\Debug\consumer.exe
curl.exe -i -H 'Content-Type: application/json' --data-binary "@$scenarioFile" http://127.0.0.1:8085/api/readings
```

Både adaptern och direkt POST ska få HTTP 201. För `extra_field.json` visar direkt POST att även API:t självt ignorerar `room`. Adaptern tar också bort okända fält när den serialiserar sin interna `Reading`.

Avsluta blocket genom att återställa normalvärdet:

```powershell
.\build\Debug\bridge.exe testdata/valid.json
```

**11:10–11:20: Paus** enligt körschemat. Låt broker och API fortsätta köra. Mottagaren ska vara avslutad.

### E. Fel innehållstyp och för stor body, 11:20–11:30

API:t i T2 ska vara igång och ha normalvärdet från D. Skicka giltig JSON med fel header direkt från T4:

```powershell
curl.exe -i -H 'Content-Type: text/plain' --data-binary '@testdata/valid.json' http://127.0.0.1:8085/api/readings
```

Förväntat: HTTP 415. Använd curl, eftersom `bridge.exe` alltid sätter `application/json` och därför inte kan skapa detta headerscenario.

Skapa därefter en tillfällig fil på exakt 4097 byte i byggmappen och skicka den direkt till API:t:

```powershell
$oversizedBody = [System.Text.Encoding]::ASCII.GetBytes('x' * 4097)
[System.IO.File]::WriteAllBytes((Join-Path $PWD 'build/oversized.txt'), $oversizedBody)
curl.exe -i -H 'Content-Type: application/json' --data-binary '@build/oversized.txt' http://127.0.0.1:8085/api/readings
.\build\Debug\consumer.exe
```

Förväntat: HTTP 413 från HTTP-bibliotekets storleksgräns och fortsatt `temp-01: 21.7 C` hos konsumenten. `build/oversized.txt` är avsiktligt en stor transport-body, inte ett giltigt JSON-exempel. Med adaptern skulle dess egen storlekskontroll stoppa filen före anropet och inget HTTP 413 kunna visas. Bibliotekets 413-svar behöver inte ha samma JSON-felmodell som API:ts egna 400/415-svar.

### F. Fel MQTT-topic och återhämtning, 11:30–11:40

T1 och T2 fortsätter köra. Starta mottagaren på nytt i T3:

```powershell
python capture.py 'C:\Program Files\mosquitto\mosquitto_sub.exe' received.json
```

Publicera inom 15 sekunder i T4, denna gång till fel topic:

```powershell
& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1885 -t iot25/dag5/other -f testdata/valid.json
```

Invänta timeout i T3. Publiceringen kan lyckas hos brokern men matchar inte mottagarens prenumeration på `iot25/dag5/reading`. `capture.py` tömmer målfilen vid start, så `received.json` är nu tom. Kör inte adaptern med den filen. Konsumenten kan fortfarande läsa det tidigare värdet från API:t, vilket visar skillnaden mellan lagrad data och en ny lyckad överföring.

För återhämtning: kör samma mottagarkommando igen i T3. Publicera sedan till rätt topic i T4:

```powershell
& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1885 -t iot25/dag5/reading -f testdata/valid.json
```

Invänta `Received ... bytes` i T3 innan du kör:

```powershell
.\build\Debug\bridge.exe received.json
.\build\Debug\consumer.exe
```

Förväntat: HTTP 201 och normalvärdet. Meddelandet från fel topic skickas inte automatiskt om till rätt topic.

### G. API-avbrott, omstart och återhämtning, 11:40–11:50

Stoppa endast `api.exe` i T2 med Ctrl+C. Brokern i T1 fortsätter köra. Kör i T4:

```powershell
.\build\Debug\bridge.exe testdata/valid.json
$LASTEXITCODE
.\build\Debug\consumer.exe
```

Förväntat: transportfel, inget HTTP-svar och exitkod 1 från adaptern. Konsumenten får också transportfel. Använd den giltiga filen: med en ogiltig fil skulle parsern stoppa adaptern innan nätverksfelet blir synligt. På loopback ger en stängd port ofta fel direkt, så demonstrationen behöver inte vänta två sekunder trots konfigurerad timeout.

Starta `api.exe` igen i T2:

```powershell
.\build\Debug\api.exe
```

Kör konsumenten i T4 innan någon ny POST:

```powershell
.\build\Debug\consumer.exe
```

Förväntat: HTTP 404 med `no reading`. API:t har startat men minneslagret har tömts. Återställ sedan genom en uttrycklig ny sändning:

```powershell
.\build\Debug\bridge.exe testdata/valid.json
.\build\Debug\consumer.exe
```

Förväntat: HTTP 201 och `temp-01: 21.7 C`. Adaptern gör ingen automatisk retry efter det tidigare avbrottet.

### H. Avslut och teststöd, 11:50–12:00

Alla elva JSON-filer i [testdataöversikten](testdata/README.md) ingår i scenarierna ovan. Den genererade `reading.json` används för sensorflödet, `received.json` för MQTT-överlämningen och `build/oversized.txt` för storleksfelet. Kontraktstestet täcker även interna värden före sändning.

Stoppa API:t i T2 och brokern i T1 med Ctrl+C efter demonstrationen. Avsluta eventuell väntande mottagare i T3. Om du vill köra det automatiska testet, gör det först när de manuella tjänsterna har stoppats:

```powershell
python test_integration.py build/Debug 'C:\Program Files\mosquitto'
```

Testet startar och avslutar egna tjänster. Kör det som förberedelse eller reservkontroll, inte samtidigt som den manuella demon använder samma portar. Testets retained-publicering är ett sätt att göra testkörningen deterministisk och skiljer sig från den manuella körningens startordning.

Avsluta med att rita varje TCP-anslutnings initiativtagare. Detta blir konkret underlag för dag 6:s brandväggsregler. TLS, autentisering och hemligheter fördjupas dag 7.
