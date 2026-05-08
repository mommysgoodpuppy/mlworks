@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Rebuild and package the Windows RTS used by MLWorks-reboot.
rem Usage:
rem   tools\reboot-rts.cmd              build, package, and smoke-test
rem   tools\reboot-rts.cmd build        build only
rem   tools\reboot-rts.cmd package      copy rebuilt artifacts only
rem   tools\reboot-rts.cmd smoke        run hello-world smoke test only
rem   tools\reboot-rts.cmd all          same as default

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=all"

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

set "RTS=%ROOT%\src\rts"
set "REBOOT=%ROOT%\MLWorks-reboot"
set "VCVARS=%VCVARS%"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

set "GNUWIN32_BIN=%GNUWIN32_BIN%"
if "%GNUWIN32_BIN%"=="" set "GNUWIN32_BIN=C:\Program Files (x86)\GnuWin32\bin"

set "MSYS2_MINGW32_BIN=%MSYS2_MINGW32_BIN%"
if "%MSYS2_MINGW32_BIN%"=="" set "MSYS2_MINGW32_BIN=C:\msys64\mingw32\bin"

set "MSYS2_USR_BIN=%MSYS2_USR_BIN%"
if "%MSYS2_USR_BIN%"=="" set "MSYS2_USR_BIN=C:\msys64\usr\bin"

set "DLLBASE=%DLLBASE%"
if "%DLLBASE%"=="" set "DLLBASE=0x18000000"

set "RTS_INCLUDES=-Igen/I386/NT -Isrc/OS/NT/arch/I386 -Isrc/OS/NT -Isrc/OS/Win32 -Isrc/OS/Win32/arch/I386 -Isrc/arch/I386 -Isrc"
set "RTS_DEFINES=-DMACH_FIXUP -DOS_NT -DWIN32 -DNATIVE_THREADS -DLITTLE_ENDIAN -DSPACE_PROFILE_OVERFLOW"
set "RTS_CFLAGS=-m32 -std=gnu89 %RTS_INCLUDES% %RTS_DEFINES%"
set "RTS_DEPFLAGS=%RTS_CFLAGS% -MM -D_M_IX86 -D_WIN32"
set "RTS_LIBS=kernel32.lib user32.lib gdi32.lib comdlg32.lib winspool.lib version.lib winmm.lib wsock32.lib advapi32.lib comctl32.lib shell32.lib"

if /I "%ACTION%"=="build" goto build
if /I "%ACTION%"=="package" goto package
if /I "%ACTION%"=="smoke" goto smoke
if /I "%ACTION%"=="all" goto all
goto usage

:all
call :build_rts || exit /b !ERRORLEVEL!
call :package_rts || exit /b !ERRORLEVEL!
call :smoke_test || exit /b !ERRORLEVEL!
exit /b 0

:build
call :build_rts
exit /b !ERRORLEVEL!

:package
call :package_rts
exit /b !ERRORLEVEL!

:smoke
call :smoke_test
exit /b !ERRORLEVEL!

:build_rts
echo == MLWorks RTS build ==
echo root:    %ROOT%
echo dllbase: %DLLBASE%

if not exist "%RTS%\afxres.h" (
  echo #include ^<winres.h^> > "%RTS%\afxres.h"
)

where cl >nul 2>nul
if errorlevel 1 (
  if not exist "%VCVARS%" (
    echo Missing Visual Studio vcvarsall.bat:
    echo   %VCVARS%
    exit /b 2
  )
  call "%VCVARS%" x86 >nul || exit /b !ERRORLEVEL!
)

set "PATH=%GNUWIN32_BIN%;%MSYS2_MINGW32_BIN%;%MSYS2_USR_BIN%;%PATH%"
where make >nul 2>nul
if errorlevel 1 (
  echo Missing make. Set GNUWIN32_BIN or install GnuWin32 make.
  exit /b 2
)

pushd "%RTS%" || exit /b 2
make ARCH=I386 OS=NT runtime DLLBASE=%DLLBASE% ^
  GCCFLAGS="%RTS_CFLAGS%" ^
  GCCFLAGSDEBUG="%RTS_CFLAGS% -g" ^
  DEPENDGENFLAGS="%RTS_DEPFLAGS%" ^
  DLLLIBRARIES="" ^
  LIBRARIES="%RTS_LIBS%" ^
  LINKENDFLAGS="/link /FORCE:MULTIPLE"
set "STATUS=%ERRORLEVEL%"
popd
exit /b %STATUS%

:package_rts
echo == MLWorks RTS package ==
if not exist "%RTS%\libmlw.dll" (
  echo Missing rebuilt DLL: %RTS%\libmlw.dll
  exit /b 2
)
if not exist "%RTS%\bin\I386\NT\main.exe" (
  echo Missing rebuilt runtime: %RTS%\bin\I386\NT\main.exe
  exit /b 2
)
copy /Y "%RTS%\libmlw.dll" "%REBOOT%\compiler\libmlw.dll" >nul || exit /b !ERRORLEVEL!
copy /Y "%RTS%\libmlw.dll" "%REBOOT%\bin\I386\NT\libmlw.dll" >nul || exit /b !ERRORLEVEL!
copy /Y "%RTS%\bin\I386\NT\main.exe" "%REBOOT%\bin\I386\NT\main.exe" >nul || exit /b !ERRORLEVEL!
echo packaged rebuilt main.exe and libmlw.dll
exit /b 0

:smoke_test
echo == MLWorks reboot smoke test ==
pushd "%REBOOT%" || exit /b 2
mlw.exe run examples\hi.mlb
set "STATUS=%ERRORLEVEL%"
popd
exit /b %STATUS%

:usage
echo Usage:
echo   tools\reboot-rts.cmd [all^|build^|package^|smoke]
echo.
echo Environment overrides:
echo   VCVARS=path\to\vcvarsall.bat
echo   GNUWIN32_BIN=path\to\GnuWin32\bin
echo   MSYS2_MINGW32_BIN=path\to\msys64\mingw32\bin
echo   MSYS2_USR_BIN=path\to\msys64\usr\bin
echo   DLLBASE=0x18000000
exit /b 2
