# REST-demo för dag 3

Windows, i Developer PowerShell for Visual Studio:

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\rest_api.exe
```

Linux:

```bash
cmake -S . -B build
cmake --build build
./build/rest_api
```

CMake hämtar fastlåsta beroenden första gången projektet konfigureras. Nätåtkomst behövs då. Testa enligt [lärardemonstrationen](../02_teori_och_larardemo.md)
