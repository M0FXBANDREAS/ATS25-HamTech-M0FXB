@echo off
set PORT=COM10
if not exist backup mkdir backup
echo READ ONLY: backing up complete detected flash from %PORT%...
python -m esptool -p %PORT% read-flash 0 ALL backup\ATS25-ORIGINAL-BACKUP.bin
if errorlevel 1 goto fail
echo Backup complete. Keep this file private and safe.
pause
exit /b 0
:fail
echo Backup failed. NOTHING WAS ERASED OR WRITTEN.
pause
exit /b 1
