# MQTTS-broker på lärarlaptopen

Den här konfigurationen låter ESP32-enheter på samma lokala nät ansluta till Mosquitto på TCP-port 8883. Brokern lyssnar på laptopens alla IPv4-interface men kräver TLS och användarnamn/lösenord.

Den ska inte exponeras mot internet. Använd ett betrott klassrumsnät, begränsa brandväggen till dess subnät och stäng processen efter laborationen.

## 1. Hitta laptopens LAN-IP

Linux:

```bash
ip -brief -4 address
```

Windows PowerShell:

```powershell
Get-NetIPConfiguration
```

Välj adressen på samma Wi-Fi/Ethernet som ESP32-enheterna, inte `127.0.0.1`, Docker-, VPN- eller VM-adresser. Exemplet nedan använder `192.168.1.42`.

Laptopens IP bör vara stabil under lektionen. En DHCP-reservation är bäst. Om IP-adressen ändras måste certifikatet genereras om och ESP32-projekten byggas om.

## 2. Skapa certifikat och användare

Från projektmappen:

```bash
cd dag_8/demo_esp32_mqtts/laptop_broker
chmod +x prepare_broker.sh start_broker.sh
./prepare_broker.sh 192.168.1.42
```

Skriptet frågar efter ett lösenord för användaren `day8`. Använd ett tillfälligt labblösenord, inte ett personligt lösenord. Följande skapas lokalt och undantas från Git:

* CA-nyckel och servernyckel i `laptop_broker/generated/`
* Hashad lösenordsfil i `laptop_broker/generated/`
* Offentligt CA-certifikat i `main/certs/laptop-ca.crt`

Endast `laptop-ca.crt` ska byggas in i ESP32-firmware.

## 3. Öppna brandväggen för klassrumsnätet

Ta reda på subnätet, exempelvis `192.168.1.0/24`. Begränsa källan till detta nät.

UFW:

```bash
sudo ufw allow from 192.168.1.0/24 to any port 8883 proto tcp comment 'IOT25 Day 8 MQTTS'
sudo ufw status
```

Firewalld, tillfällig regel som försvinner vid reload/omstart:

```bash
sudo firewall-cmd --add-rich-rule='rule family="ipv4" source address="192.168.1.0/24" port port="8883" protocol="tcp" accept'
sudo firewall-cmd --list-rich-rules
```

Anpassa subnätet. Öppna inte port 8883 för `0.0.0.0/0` och skapa ingen port-forward i routern.

## 4. Starta brokern

Terminal 1 – Lärarlaptop, broker/gateway:

```bash
cd dag_8/demo_esp32_mqtts/laptop_broker
./start_broker.sh
```

Skriptet kör i förgrunden så att anslutningar och fel syns under demonstrationen. Det installerar eller ändrar inte Mosquittos systemtjänst.

Kontrollera i en annan terminal att processen lyssnar:

```bash
ss -ltn '( sport = :8883 )'
```

## 5. Testa lokalt

Terminal 2 – Lärarlaptop, testkonsument:

```bash
cd dag_8/demo_esp32_mqtts/laptop_broker
mosquitto_sub -h 192.168.1.42 -p 8883 \
  --cafile generated/ca.crt \
  -u day8 -P 'LAB_PASSWORD' \
  -t 'iot25/day8/#' -v
```

I terminal 3 kan ett testmeddelande publiceras innan ESP32 ansluts:

```bash
mosquitto_pub -h 192.168.1.42 -p 8883 \
  --cafile generated/ca.crt \
  -u day8 -P 'LAB_PASSWORD' \
  -t 'iot25/day8/teacher-test' \
  -m '{"status":"ok"}'
```

Lösenord i kommandoraden kan synas i historik eller processlista. Kommandona ovan är endast för ett tillfälligt labbkonto. Radera eller byt kontot efter laborationen.

Testa därefter från en annan dator på samma nät. Om lokalt test fungerar men fjärrtest misslyckas är brandvägg, fel IP eller klientisolering i Wi-Fi vanligast.

## 6. Konfigurera studenternas ESP32-projekt

Varje student behöver samma offentliga `main/certs/laptop-ca.crt`. De gemensamma värdena delas i `sdkconfig.defaults`. Kontrollera eller uppdatera följande före utdelning:

* Broker URI: `mqtts://192.168.1.42:8883`
* MQTT username: `day8`
* MQTT password: Det tillfälliga labblösenordet
* Topic prefix: Ett unikt prefix, exempelvis `iot25/day8/grupp-04`

Studenterna genererar en lokal `sdkconfig` med `idf.py reconfigure` eller `idf.py set-target esp32c6`. Bygg om efter att CA-filen, `sdkconfig.defaults` eller broker-IP har ändrats.

## 7. Stäng och återställ

Stoppa brokern med `Ctrl+C`. Ta bort den tillfälliga brandväggsregeln när laborationen är klar.

UFW:

```bash
sudo ufw delete allow from 192.168.1.0/24 to any port 8883 proto tcp
```

Firewalld:

```bash
sudo firewall-cmd --remove-rich-rule='rule family="ipv4" source address="192.168.1.0/24" port port="8883" protocol="tcp" accept'
```

De privata nycklarna kan behållas för nästa pass om laptopens IP är oförändrad. Om de tas bort måste ESP32-enheterna få det nya CA-certifikatet vid nästa körning.
