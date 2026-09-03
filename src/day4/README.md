# JSON- och XML-demo för dag 4

Windows, i Developer PowerShell for Visual Studio:

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\json_demo.exe
.\build\Debug\xml_demo.exe
```

Linux:

```bash
cmake -S . -B build
cmake --build build
./build/json_demo
./build/xml_demo
```

CMake hämtar fastlåsta beroenden första gången projektet konfigureras.
