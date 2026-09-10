# Isolerad nätverkslabb – Routing och nftables

Miljön skapar fyra Linux network namespaces: IoT, tjänster, administration och router. Varje klientnät har en virtuell kabel till routern. Ingen kabel ansluts till värddatorns nät. Skriptet ändrar forwarding och brandvägg endast inne i routerns namespace.

Detta är riktig IP-routing och paketfiltrering. Port 1883 och 8080 har enkla TCP-testtjänster som returnerar en kontrolltext. De implementerar inte MQTT eller HTTP. Tester visar nätverksåtkomst, och dag 5:s flöde visar applikationsintegration. Båda nivåerna behöver testas i caset.

## Förbered Linux eller WSL2

Använd Ubuntu/Debian i en disponibel labb-VM eller WSL2 med stöd för network namespaces och nftables. Du behöver sudo-rättigheter i Linux. Installera i förväg:

```bash
sudo apt-get update
sudo apt-get install iproute2 nftables python3
```

Öppna sedan en Linuxterminal i denna mapp. I WSL ligger Windowsprojektet under `/mnt/c/...`. Använd citattecken kring sökvägen om den innehåller mellanslag. Kör alla kommandon nedan i Linuxterminalen, inte direkt i PowerShell.

Behåll WSL-terminalen öppen under hela labben. Om WSL avslutas försvinner körande processer och virtuella nät. En kvarvarande `.state/owner` efter omstart hanteras med `sudo bash lab.sh down` före en ny `up`.

## IP-plan

| Namespace | Klientadress | Routerns gränssnitt | Gateway |
|---|---|---|---|
| `iot25-d6-iot` | 10.60.10.10/24 | `r-iot`, 10.60.10.1/24 | 10.60.10.1 |
| `iot25-d6-srv` | 10.60.20.10/24 | `r-srv`, 10.60.20.1/24 | 10.60.20.1 |
| `iot25-d6-admin` | 10.60.30.10/24 | `r-admin`, 10.60.30.1/24 | 10.60.30.1 |

## Start och grundtest

```bash
sudo bash lab.sh up
sudo bash lab.sh status
sudo bash lab.sh test
```

Sex rader ska visa PASS. De två första provar tjänsterna från tjänstenätet så att en stoppad server inte misstas för en fungerande brandvägg. Därefter provas både tillåten och blockerad trafik från IoT och administration.

```text
PASS srv -> 1883 ALLOW expected ALLOW
PASS srv -> 8080 ALLOW expected ALLOW
PASS iot -> 1883 ALLOW expected ALLOW
PASS iot -> 8080 BLOCK expected BLOCK
PASS admin -> 8080 ALLOW expected ALLOW
PASS admin -> 1883 BLOCK expected BLOCK
```

Varje probe använder en ny TCP-anslutning med en sekunds timeout. Ett BLOCK visar att anslutningen eller kontrolltexten inte nådde fram. Läs även reglernas räknare för att styrka att just filtreringen orsakade utfallet.

## Säkerhetskopia, fel och återställning

```bash
sudo bash lab.sh backup
sudo bash lab.sh break
sudo bash lab.sh test-broken
sudo bash lab.sh restore
sudo bash lab.sh test
```

Efter `break` ska lokal tjänsteåtkomst fortfarande fungera men all testad trafik genom routern blockeras. `test-broken` ger PASS när just det felbeteendet observeras. Efter `restore` ska det ordinarie testet ge sex PASS igen.

Backupfilen `.state/backup.nft` innehåller routerns regler. Adresser och rutter återskapas av `lab.sh up` och ingår inte i nftables-backupen. Kör `status` och dokumentera dem separat om du ändrar topologin. En fullständig enhetsbackup behöver omfatta mer än brandväggsregler.

## Inspektera en sträcka

```bash
sudo ip -n iot25-d6-iot addr
sudo ip -n iot25-d6-iot route
sudo ip netns exec iot25-d6-router sysctl net.ipv4.ip_forward
sudo ip netns exec iot25-d6-router nft list ruleset
```

Reglerna har standardpolicyn drop för vidarebefordrad trafik. `ct state established,related` släpper tillbaka svar som tillhör tillåtna anslutningar. Nya anslutningar prövas mot källadress, måladress och målport. Regeln gäller `forward`, eftersom paketet passerar routern och ska till en annan dator.

ICMP mellan segment är inte tillåtet i grundkonfigurationen. Ett misslyckat ping betyder därför inte att tillåten TCP-trafik är trasig. Routerns lokala input-kedja omfattas inte av övningens forward-policy. Administration av själva routern behöver en separat input-policy i en driftmiljö.

## Avslut

```bash
sudo bash lab.sh down
```

Detta stoppar processer och tar bort endast labbens namngivna namespaces. Backup och logg ligger kvar. Skriptet vägrar starta över befintliga namn. Om starten avbryts abrupt, läs status och kör `down` före en ny start.

## Reservspår

Om kernelstöd eller sudo saknas: arbeta i par med en kurskamrat som kör miljön, eller använd lärarens skärmdelade labb. Den ena skriver regeländringen och förutsäger resultatet, den andra kör den. Spara verkliga observationer. Enbart en pappersskiss övar planering men behöver kompletteras med konfiguration och test för dagens praktiska mål.

## Lärarens automatiska genomkörning

När ingen labb redan körs kan hela kedjan, inklusive studentens byte av målport, testas med:

```bash
sudo bash selftest.sh
```

Två avsiktliga FAIL-rader ska visas under studentändringen och kontrolleras av skriptet. Alla ordinarie krav ska vara uppfyllda efter återställning. Skriptet avslutar de egna labbnäten även om ett test misslyckas efter starten.
