# CA-certifikat för laptopbrokern

`laptop-ca.crt` skapas av `laptop_broker/prepare_broker.sh` och kopieras hit automatiskt. Filen är offentlig men lokal för labbmiljön och undantas från Git eftersom den genereras för aktuell broker.

Brokerns privata filer `ca.key` och `server.key` får aldrig kopieras till denna mapp eller till ESP32-enheten.
