@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Rebuild the MLWorks-reboot command-line launcher.
rem Usage:
rem   tools\reboot-mlw.cmd              build and smoke-test
rem   tools\reboot-mlw.cmd build        build only
rem   tools\reboot-mlw.cmd smoke        run smoke tests only
rem   tools\reboot-mlw.cmd all          same as default

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

set "REBOOT=%ROOT%\MLWorks-reboot"
set "VCVARS=%VCVARS%"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

if /I "%ACTION%"=="build" goto build
if /I "%ACTION%"=="smoke" goto smoke
if /I "%ACTION%"=="all" goto all
goto usage

:all
call :build_mlw || exit /b !ERRORLEVEL!
call :smoke_test || exit /b !ERRORLEVEL!
exit /b 0

:build
call :build_mlw
exit /b !ERRORLEVEL!

:smoke
call :smoke_test
exit /b !ERRORLEVEL!

:build_mlw
echo == MLWorks launcher build ==
echo root: %ROOT%

where cl >nul 2>nul
if errorlevel 1 (
  if not exist "%VCVARS%" (
    echo Missing Visual Studio vcvarsall.bat:
    echo   %VCVARS%
    exit /b 2
  )
  call "%VCVARS%" x86 >nul || exit /b !ERRORLEVEL!
)

if not exist "%REBOOT%\mlw.c" (
  echo Missing launcher source: %REBOOT%\mlw.c
  exit /b 2
)

pushd "%REBOOT%" || exit /b 2
cl /nologo /W3 /O2 /MT /Fe:mlw.exe mlw.c shlwapi.lib
set "STATUS=%ERRORLEVEL%"
if exist mlw.obj del /q mlw.obj
popd
exit /b %STATUS%

:smoke_test
echo == MLWorks launcher smoke test ==
pushd "%REBOOT%" || exit /b 2
mlw.exe run examples\hi.mlb
if errorlevel 1 (
  set "STATUS=%ERRORLEVEL%"
  popd
  exit /b !STATUS!
)
mlw.exe run examples\basis.mlb 40
if errorlevel 1 (
  set "STATUS=%ERRORLEVEL%"
  popd
  exit /b !STATUS!
)
mlw.exe run examples\modules\main.mlb
set "STATUS=%ERRORLEVEL%"
popd
exit /b %STATUS%

:usage
echo Usage:
echo   tools\reboot-mlw.cmd [all^|build^|smoke]
echo.
echo Environment overrides:
echo   VCVARS=path\to\vcvarsall.bat
exit /b 2
