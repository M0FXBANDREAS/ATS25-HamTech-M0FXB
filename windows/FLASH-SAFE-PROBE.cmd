@echo off
setlocal
cd /d "%~dp0"
echo ATS-25 HamTech M0FXB V0.2 SAFE PROBE FLASHER
echo.
set /p PORT=Enter COM port [COM10]: 
if "%PORT%"=="" set PORT=COM10
python -m esptool --chip esp32 -p %PORT% flash-id || goto :fail
echo.
echo This will replace the currently booted application. Your 4MB factory backup is required for recovery.
choice /C YN /M "Continue"
if errorlevel 2 exit /b 0
if not exist "..\firmware\ATS25-HamTech-M0FXB-V0.2-FULL.bin" (
  echo Firmware file not found. Copy the GitHub Actions artifact files into a folder named firmware next to this windows folder.
  pause
  exit /b 1
)
python -m esptool --chip esp32 -p %PORT% --baud 460800 write-flash 0x0 "..\firmware\ATS25-HamTech-M0FXB-V0.2-FULL.bin" || goto :fail
echo Flash complete. Reboot the ATS-25 and open a 115200 baud serial monitor.
pause
exit /b 0
:fail
echo FAILED. Nothing further was written by this script after the failed command.
pause
exit /b 1
