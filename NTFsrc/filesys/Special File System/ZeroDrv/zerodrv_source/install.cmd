@echo off
net stop zero > nul 2>&1
echo Installing Zero And Random Device Driver...
rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 .\zero.inf
echo.
echo Loading Zero And Random Device Driver...
net start zero > nul 2>&1
if errorlevel 1 (
  echo.
  echo Error installing/loading the driver. Check %SystemRoot%\setupapi.log for details.
) else (
  echo.
  echo The Zero And Random Device Driver was successfully loaded into the kernel.
)
echo.
pause
