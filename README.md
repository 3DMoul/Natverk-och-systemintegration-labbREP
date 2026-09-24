# git-recap
detta reposetory är för alla labbar
day9


server.cpp
---
HTTP-status: 400 Bad Request
Loggnivå: WARNING
eventnamn: reading_rejected
Felorsak: value must be a finite number
Svarstid: 0.246 ms

server.py
---
HTTP-status: 400 Bad Request
Loggnivå: WARNING
eventnamn: reading_rejected
Felorsak: value must be a finite number
Svarstid: 0.108 ms


2.Kontrollera mätetal
---
Öppna http://127.0.0.1:8090/dashboard. Jämför dashboarden med JSON-svaret och textformatet.
Förklara varför tre vyer inte betyder tre oberoende mätningar.

:det tre vyer betyder inte tre oberoende mätningar. det är tre olika sätt att presentera samma mätdata från servern.


man kan se det som:
Dashboard → lätt för en människa att snabbt se status och upptäcka avvikelser.
JSON → bra för andra program/API-klienter som behöver läsa och bearbeta datan.
Textformat → bra för övervakningsverktyg som exempelvis Prometheus.

4.Filtrera nätverkstrafik
---
Om trafiken i stället gick över HTTPS/TLS skulle Wireshark fortfarande kunna se:

IP-adresser
TCP-portar
Att en TLS-anslutning används
Mängden data som skickas

Men innehållet skulle vara krypterat. Du skulle inte kunna läsa:

HTTP-metoden (GET, POST)
URL:er som /api/readings eller /api/metrics
HTTP-statuskoder (202, 400, 404)
HTTP-headerar
JSON-data och mätvärden
Request-ID:n och annan applikationsdata