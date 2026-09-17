# C++-simulator – Dag 8

Simulatorn visar två periodiska sensorer, en begränsad kö, ett simulerat nätavbrott och återanslutning. Den använder ingen broker och behöver bara en C++17-kompilator.

## Representerad enhet

Terminal 1 representerar en ESP32-C6 som innehåller:

* Två timerstyrda sensorkällor
* En händelsekö
* En publiceringsuppgift
* Ett anslutningstillstånd
* Räknare för skapade, publicerade och tappade händelser

## Bygg och kör

Linux:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic simulator.cpp -o simulator
./simulator
```

Windows med MSVC Developer PowerShell:

```powershell
cl /std:c++17 /W4 /EHsc simulator.cpp
.\simulator.exe
```

Programmet använder simulerad tid och avslutas direkt. Förväntad slutrad är:

```text
SUMMARY created=11 published=7 dropped=4 reconnects=1
```

Händelser som skapas under avbrottet tappas enligt simulatorns uttalade policy. Det är inte den enda möjliga policyn.
