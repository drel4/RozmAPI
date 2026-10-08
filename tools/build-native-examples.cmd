@echo off
setlocal
cd /d "%~dp0.."
set "ROZM_VCVARS=%VCVARSALL%"
if not defined ROZM_VCVARS set "ROZM_VCVARS=E:\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
call "%ROZM_VCVARS%" x86 >nul
if errorlevel 1 exit /b 1
cl /nologo /W4 /MT examples\native.c build\win32\RozmAPI.lib /Fobuild\win32\native.obj /Febuild\win32\native-example.exe
if errorlevel 1 exit /b 1
call "%ROZM_VCVARS%" x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /W4 /MT examples\native.c build\win64\RozmAPI.lib /Fobuild\win64\native.obj /Febuild\win64\native-example.exe
exit /b %errorlevel%
