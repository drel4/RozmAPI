@echo off
setlocal
cd /d "%~dp0"
set "ROZM_VCVARS=%VCVARSALL%"
if not defined ROZM_VCVARS set "ROZM_VCVARS=E:\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%ROZM_VCVARS%" (
  echo Set VCVARSALL to the path of Visual Studio vcvarsall.bat.
  exit /b 1
)
if not exist build\win32 mkdir build\win32
if not exist build\win64 mkdir build\win64
call "%ROZM_VCVARS%" x86 >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /MT /O2 /W4 src\worker32.cpp /Febuild\win32\RozmWorker32.exe /Fobuild\win32\ /link bcrypt.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /MT /O2 /W4 /LD src\api.cpp /Febuild\win32\RozmAPI.dll /Fobuild\win32\ /link /DEF:src\RozmAPI.def
if errorlevel 1 exit /b 1
copy /y build\win32\RozmWorker32.exe build\win64\RozmWorker32.exe >nul
call "%ROZM_VCVARS%" x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /MT /O2 /W4 /LD src\api.cpp /Febuild\win64\RozmAPI.dll /Fobuild\win64\ /link /DEF:src\RozmAPI.def
exit /b %errorlevel%
