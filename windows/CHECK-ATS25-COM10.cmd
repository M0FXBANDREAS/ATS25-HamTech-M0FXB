@echo off
set PORT=COM10
echo ATS-25 HamTech M0FXB Controller - read-only hardware check
echo Port: %PORT%
python -m esptool -p %PORT% flash-id
pause
