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

---

Del B – Baslinje

1. Kör python client.py --count 20 --interval-ms 50
2. Spara antal försök, lyckade, fel, min, median, medel, p95 och max
3. Hämta curl http://127.0.0.1:8091/metrics
4. Beskriv exakt vad klientens svarstid mäter
5. Upprepa körningen och notera normal variation

---

Del C – Kontrollerad fördröjning

1. Stoppa servern
2. Starta python server.py --delay-ms 250
3. Skriv en hypotes innan klienten körs
4. Kör samma klientkommando som i baslinjen
5. Jämför klientens totaltid med serverns processing_ms
6. Ange vilket bevis som stödjer eller motsäger hypotesen

---

Del D – Skilj tre feltyper åt

Genomför testerna ett i taget och återställ mellan dem. Test Så framkallas felet Fråga

- Anslutningsfel | Kör klienten mot port 8092 | Finns HTTP-status eller serverlogg?
- Serverfel | Starta servern med --failure-every 3 | Vilka anrop får status 500?
- Valideringsfel | Kör klienten med --invalid-every 3 | Varför är status 400 inte paketförlust?

För varje test ska ni spara:
- Symptom från klienten
- Relevant serverlogg eller frånvaro av serverlogg
- Ett ytterligare bevis, exempelvis mätetal, ss eller nätverksspår
- Klassificering av felgränsen

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
