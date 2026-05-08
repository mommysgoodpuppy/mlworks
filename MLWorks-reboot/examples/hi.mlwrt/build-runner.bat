@echo off
where cl >nul 2>nul
if errorlevel 1 (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
)
cl /nologo /W3 /O2 /MT /Fe:"C:\GIT\mlworks\MLWorks-reboot\examples\hi.exe" "C:\GIT\mlworks\MLWorks-reboot\examples\hi.mlwrt\mlw-runner.c"
exit /b %ERRORLEVEL%
