@echo off
echo Building Nightreign Status Mod (Release)...

rmdir /s /q build 2>nul
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

echo.
echo Build complete!
echo Output: build\bin\Release\NightreignStatusMod.dll
pause