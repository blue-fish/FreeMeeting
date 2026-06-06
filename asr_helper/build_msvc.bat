@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\WeNet\asr_helper\build
cmake .. -G "Visual Studio 16 2019" -A x64
if %ERRORLEVEL% EQU 0 cmake --build . --config Release