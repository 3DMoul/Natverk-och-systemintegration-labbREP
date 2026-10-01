# git-recap
detta reposetory är för alla labbar
day10


Arbetsform och roller

---

Del A – Startkontroll

1. Välj Python-demot eller C++-demot och starta servern enligt dess README
2. Verifiera hälsa med curl http://127.0.0.1:8091/health
3. Kör tre begäranden och kontrollera att både klient och server visar samma request_id
4. Skriv version, port och startkommando i protokollet
version: python
port: 8091
startkommando: python client.py --count 5 --interval-ms 50 

---

Del B – Baslinje

1. Kör python client.py --count 20 --interval-ms 50
2. Spara antal försök, lyckade, fel, min, median, medel, p95 och max
<!-- {
  "attempts": 20,
  "successes": 20,
  "failures": 0,
  "error_fraction": 0.0,
  "elapsed_s": 1.253,
  "throughput_success_per_s": 15.965,
  "latency_ms_min": 1.491,
  "latency_ms_avg": 14.66,
  "latency_ms_p95": 26.687,
  "latency_ms_max": 27.107,
  "errors": {}
} -->
3. Hämta curl http://127.0.0.1:8091/metrics
<!-- StatusCode        : 200
StatusDescription : OK
Content           : {"requests_total": 30, "accepted_total": 30, "validation_errors_total": 0, "server_errors_total": 0, 
                    "processing_ms_avg": 0.057, "processing_ms_max": 0.101}
RawContent        : HTTP/1.0 200 OK
                    Content-Length: 156
                    Content-Type: application/json
                    Date: Thu, 24 Sep 2026 14:35:26 GMT
                    Server: BaseHTTP/0.6 Python/3.13.12
                    
                    {"requests_total": 30, "accepted_total": 30, "validati...
Forms             : {}
Headers           : {[Content-Length, 156], [Content-Type, application/json], [Date, Thu, 24 Sep 2026 14:35:26 GMT], [Server, 
                    BaseHTTP/0.6 Python/3.13.12]}
Images            : {}
InputFields       : {}
Links             : {}
ParsedHtml        : mshtml.HTMLDocumentClass
RawContentLength  : 156 -->
4. Beskriv exakt vad klientens svarstid mäter
den mäter tiden mellan att klienten har skickat HTTP-begäran tills den har fått tillbacka HTTP-svaret från servern. medan serverns processing_ms endast mäter serverns egen behandlingstid.
5. Upprepa körningen och notera normal variation
Normal variation: Vid två identiska körningar med 20 requests var alla requests lyckade och svarstiderna varierade något. Medelvärdet var 15,690 ms respektive 14,873 ms och P95 var 27,175 ms respektive 26,987 ms. Maxvärdet varierade mer, från 27,723 ms till 42,308 ms. Detta visar att svarstiden kan variera mellan körningar även när systemet körs under samma förhållanden.
---

Del C – Kontrollerad fördröjning

1. Stoppa servern
2. Starta python server.py --delay-ms 250
3. Skriv en hypotes innan klienten körs
4. Kör samma klientkommando som i baslinjen
5. Jämför klientens totaltid med serverns processing_ms
6. Ange vilket bevis som stödjer eller motsäger hypotesen
3. Hypotes

Jag förväntade mig att serverns `processing_ms` skulle vara ungefär samma, medan klientens svarstid skulle öka på grund av fördröjningen på 250 ms.

5. Jämförelse

I baslinjen var klientens genomsnittliga svarstid 15,690 ms och serverns `processing_ms` var cirka 0,057 ms.

Efter att servern startades med `--delay-ms 250` blev klientens genomsnittliga svarstid 271,931 ms. Serverns `processing_ms_avg` blev 250,456 ms och `processing_ms_max` blev 250,861 ms.

6. Bevis

Resultaten visar att klientens svarstid ökade kraftigt när servern fick en fördröjning på 250 ms. Serverns processing-tid ökade samtidigt från cirka 0,057 ms till cirka 250,456 ms. Detta visar att den tillagda serverfördröjningen är den huvudsakliga orsaken till den ökade svarstiden.

Hypotesen stöds därför när det gäller att klientens svarstid ökar, men den första delen av hypotesen stämmer inte eftersom serverns `processing_ms` också ökade på grund av den tillagda fördröjningen.

---

Del D – Skilj tre feltyper åt

Genomför testerna ett i taget och återställ mellan dem. Test Så framkallas felet Fråga

- Anslutningsfel | Kör klienten mot port 8092 | Finns HTTP-status eller serverlogg?
- Serverfel | Starta servern med --failure-every 3 | Vilka anrop får status 500?
- Valideringsfel | Kör klienten med --invalid-every 3 | Varför är status 400 inte paketförlust?

För varje test ska ni spara:
- Symptom från klienten
Test 1 – Anslutningsfel

- Relevant serverlogg eller frånvaro av serverlogg
- Ett ytterligare bevis, exempelvis mätetal, ss eller nätverksspår
- Klassificering av felgränsen

Kommando:

python client.py --port 8092 --count 3

Symptom från klienten:

Alla 3 försök misslyckades.
Klienten fick error=<urlopen error timed out>.
status=None för samtliga försök.
Felklassen blev connection_or_timeout.
error_fraction=1.0, alltså 100 % misslyckade försök.
Genomsnittlig väntetid var cirka 2015 ms.

Relevant serverlogg / frånvaro av serverlogg:

Eftersom klienten inte fick någon HTTP-status (status=None) finns inget HTTP-svar att dokumentera.
Om serverloggen samtidigt saknar motsvarande HTTP-anrop är det ytterligare stöd för att felet sker innan HTTP-begäran når applikationen.

Test 2 – Serverfel

Kommando:

python client.py --count 20 --interval-ms 50

Detta test visar tydligt mönstret från --failure-every 3.

Vilka anrop fick status 500?

Anrop	Status
003	500
006	500
009	500
012	500
015	500
018	500

Alltså var tredje anrop, totalt 6 av 20.

Symptom från klienten:

14 anrop lyckades med 202.
6 anrop fick 500.
Alla fel klassificerades som http_500.
error_fraction=0.3, alltså 30 %.
Genomsnittlig latens: 19,191 ms.
P95: 26,938 ms.

Test 3 – Valideringsfel

Kommando:

python client.py --count 20 --invalid-every 3

Vilka anrop fick status 400?

Anrop	Status
003	400
006	400
009	400
012	400
015	400
018	400

Även här är det alltså var tredje anrop, totalt 6 av 20.

Symptom från klienten:

14 anrop lyckades med 202.
6 anrop fick 400.
Alla fel klassificerades som http_400.
error_fraction=0.3, alltså 30 %.
Genomsnittlig latens: 15,238 ms.

Test	        Klientsymptom	Server/HTTP-bevis	                                    Ytterligare bevis	                Felgräns
Anslutningsfel	3/3             (timeout, status=None)	                                Inget HTTP-svar	error_fraction=1.0	Anslutning/transport före HTTP
Serverfel	    6 × 500	        (HTTP 500 på anrop 3, 6, 9, 12, 15, 18)	                6/20 fel, 30 %	                    Server/applikation
Valideringsfel	6 × 400	        (HTTP 400 på anrop 3, 6, 9, 12, 15, 18)	                6/20 fel, 30 %	                    Applikation/validering

---

Del E – Åtgärd och återställning

Välj en liten åtgärd som matchar ett observerat problem. Exempel:
- Rätta porten i klientkonfigurationen
- Minska onödig behandlingstid
- Förbättra valideringsfelets meddelande
- Begränsa samtidighet eller ködjup
- Kör samma test före och efter. Dokumentera en möjlig bieffekt. Starta därefter servern utan felparametrar och verifiera baslinjen igen.

---

Valfri del F – Passiv nätverksobservation

Kör på loopbackgränssnittet:
sudo tcpdump -i lo -nn 'tcp port 8091'

På Windows kan Wireshark med Npcap loopback-adapter användas. Denna del är valfri eftersom klientresultat och serverlogg räcker för grundmålen.

Leverans
- Ifyllt mät- och hypotesprotokoll
- Minst två baslinjekörningar
- Tre klassificerade feltyper
- En före- och efterjämförelse eller en tydligt motiverad rekommendation
- Återställningsbevis

------

test av python version:
kontroll::

<!-- test_nearest_rank (test_demo.PercentileTests.test_nearest_rank) ... ok
test_unsorted_input (test_demo.PercentileTests.test_unsorted_input) ... ok

----------------------------------------------------------------------
Ran 2 tests in 0.002s

OK -->

kontroll av hälsa curl http://127.0.0.1:8091/health

<!-- StatusCode        : 200
StatusDescription : OK
Content           : {"status": "ok"}
RawContent        : HTTP/1.0 200 OK
                    Content-Length: 16
                    Content-Type: application/json
                    Date: Thu, 24 Sep 2026 14:19:33 GMT
                    Server: BaseHTTP/0.6 Python/3.13.12
                    
                    {"status": "ok"}
Forms             : {}
Headers           : {[Content-Length, 16], [Content-Type, application/json], [Date, Thu, 24 Sep 2026 14:19:33 GMT], [Server, 
                    BaseHTTP/0.6 Python/3.13.12]}
Images            : {}
InputFields       : {}
Links             : {}
ParsedHtml        : mshtml.HTMLDocumentClass
RawContentLength  : 16 -->


kontroll att request id är samma på klient och server:

klient:
<!-- day10-001 status=202 class=ok duration_ms=61.363
day10-002 status=202 class=ok duration_ms=12.417
day10-003 status=202 class=ok duration_ms=2.034
day10-004 status=202 class=ok duration_ms=20.589
day10-005 status=202 class=ok duration_ms=10.928 -->

server:

<!-- {"timestamp": "2026-09-24T14:24:00Z", "level": "INFO", "event": "accepted", "request_id": "day10-001", "status": 202, "processing_ms": 0.035}
{"timestamp": "2026-09-24T14:24:00Z", "level": "INFO", "event": "accepted", "request_id": "day10-002", "status": 202, "processing_ms": 0.062}
{"timestamp": "2026-09-24T14:24:00Z", "level": "INFO", "event": "accepted", "request_id": "day10-003", "status": 202, "processing_ms": 0.026}
{"timestamp": "2026-09-24T14:24:01Z", "level": "INFO", "event": "accepted", "request_id": "day10-004", "status": 202, "processing_ms": 0.051}
{"timestamp": "2026-09-24T14:24:01Z", "level": "INFO", "event": "accepted", "request_id": "day10-005", "status": 202, "processing_ms": 0.033} -->


Jag skickade fem begäranden från klienten. Alla begäranden fick status 202 och request_id var samma på klient- och serversidan, exempelvis day10-001 på båda sidorna. Detta visar att request_id följer begäran korrekt mellan klient och server.

