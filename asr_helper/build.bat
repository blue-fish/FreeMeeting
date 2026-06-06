@echo off
set "PATH=E:\colin\Qt\5.12.11\mingw73_64\bin;E:\colin\Qt\Tools\mingw730_64\bin;%PATH%"
cd /d "%~dp0"

echo === Cleaning old build ===
if exist release\main.o del release\main.o
if exist Makefile del Makefile
if exist Makefile.Debug del Makefile.Debug
if exist Makefile.Release del Makefile.Release

echo === Running qmake (64-bit) ===
qmake asr_helper.pro
if %errorlevel% neq 0 (
    echo QMAKE FAILED!
    pause
    exit /b 1
)

echo === Building ===
mingw32-make
if %errorlevel% neq 0 (
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo === SUCCESS ===
echo Output: ..\release\asr\sherpa_asr_helper.exe
pause
