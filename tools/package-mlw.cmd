@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build a portable-ish Windows MLWorks reboot package.
rem Usage:
rem   tools\package-mlw.cmd
rem   tools\package-mlw.cmd C:\path\to\output-dir

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

if "%~1"=="" (
  set "DIST=%ROOT%\dist\mlworks-reboot-win32"
) else (
  set "DIST=%~1"
)
for %%I in ("%DIST%") do set "DIST=%%~fI"

set "REBOOT=%ROOT%\MLWorks-reboot"
set "ZIP=%DIST%.zip"

echo == MLWorks portable package ==
echo root: %ROOT%
echo dist: %DIST%

call "%ROOT%\tools\reboot-mlw.cmd" build || exit /b !ERRORLEVEL!

pushd "%REBOOT%" || exit /b 2
mlw.exe basis || (
  set "STATUS=!ERRORLEVEL!"
  popd
  exit /b !STATUS!
)
set MLWORKS_FOREIGN=1
mlw.exe foreign || (
  set "STATUS=!ERRORLEVEL!"
  popd
  exit /b !STATUS!
)
popd

if exist "%DIST%" rmdir /s /q "%DIST%" || exit /b !ERRORLEVEL!
mkdir "%DIST%" || exit /b !ERRORLEVEL!

copy /y "%REBOOT%\mlw.exe" "%DIST%\" >nul || exit /b !ERRORLEVEL!
copy /y "%REBOOT%\mlw.bat" "%DIST%\" >nul || exit /b !ERRORLEVEL!
copy /y "%REBOOT%\README.md" "%DIST%\" >nul || exit /b !ERRORLEVEL!
copy /y "%REBOOT%\basis-objects.txt" "%DIST%\" >nul || exit /b !ERRORLEVEL!
copy /y "%REBOOT%\foreign-objects.txt" "%DIST%\" >nul || exit /b !ERRORLEVEL!

robocopy "%REBOOT%\bin" "%DIST%\bin" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%REBOOT%\compiler" "%DIST%\compiler" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%REBOOT%\images" "%DIST%\images" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%REBOOT%\examples" "%DIST%\examples" /E /XD .mlw DEPEND objects *.mlwrt /XF .DS_Store *.obj *.exe *.dll *.lib *.mo *.out *.mlp >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%

mkdir "%DIST%\lib\src" || exit /b !ERRORLEVEL!
copy /y "%ROOT%\src\basis.mlp" "%DIST%\lib\src\" >nul || exit /b !ERRORLEVEL!
robocopy "%ROOT%\src\pervasive" "%DIST%\lib\src\pervasive" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%ROOT%\src\basis" "%DIST%\lib\src\basis" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%ROOT%\src\foreign" "%DIST%\lib\src\foreign" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%ROOT%\src\win_nt" "%DIST%\lib\src\win_nt" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%ROOT%\src\rts\gen" "%DIST%\lib\src\rts\gen" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%
robocopy "%ROOT%\objects\i386\nt\release" "%DIST%\lib\objects\i386\nt\release" /E /XF .DS_Store >nul
if %ERRORLEVEL% GEQ 8 exit /b %ERRORLEVEL%

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "if (Test-Path -LiteralPath '%ZIP%') { Remove-Item -LiteralPath '%ZIP%' -Force }; Compress-Archive -LiteralPath '%DIST%' -DestinationPath '%ZIP%'" ^
  || exit /b !ERRORLEVEL!

echo Wrote %ZIP%
exit /b 0
