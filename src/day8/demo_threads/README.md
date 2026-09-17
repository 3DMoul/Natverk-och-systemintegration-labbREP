# Tråddemo på dator – Dag 8

Demot visar grunderna innan samma ansvar flyttas till FreeRTOS: två arbetstrådar uppdaterar ett delat resultat, en mutex skyddar den kritiska sektionen och huvudtråden väntar med `join`.

## Linux, macOS och WSL med pthreads

En terminal representerar en datorprocess. Processen innehåller huvudtråden och två arbetstrådar, så flera terminaler skulle inte göra ansvarsfördelningen tydligare.

```bash
cd dag_8/demo_threads
cc -std=c11 -Wall -Wextra -Wpedantic -pthread pthread_demo.c -o pthread_demo
./pthread_demo
```

Följ utskrifterna `START`, `DONE` och `RESULT`. Slutvärdet ska alltid vara `400000`.

## Windows utan WSL

Pthreads ingår inte i Windows standard-API. `windows_threads_demo.c` visar samma mönster med `CreateThread`, `CRITICAL_SECTION` och `WaitForMultipleObjects`.

Kompilera i en **Developer Command Prompt for Visual Studio**:

```bat
cd dag_8\demo_threads
cl /W4 /std:c11 windows_threads_demo.c
windows_threads_demo.exe
```

Med MinGW kan pthread-demot också fungera om verktygskedjan levererar pthreads, men Win32-varianten kräver inget separat pthread-bibliotek.

## Begrepp att peka ut

* `main` är redan en tråd
* Båda arbetstrådarna delar `shared_counter`
* Mutexen gör läsning, ändring och skrivning till en kritisk sektion
* `join` eller väntan gör att resurser inte städas bort för tidigt
* Exekveringsordningen mellan arbetstrådarna är inte garanterad
