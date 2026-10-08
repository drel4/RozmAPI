@echo off
call "E:\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /EHsc /std:c++17 /MT /Zi /Od /DROZM_PROBE src\engine32.cpp /Febuild\probe.exe /Fobuild\ /link bcrypt.lib /DEBUG
