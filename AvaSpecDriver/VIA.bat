@echo off
REM VIA Unified Console for Windows
REM
REM Interactive console with automatic session logging
REM All data automatically saved to timestamped session folders
REM
REM Usage: VIA.bat [port]
REM        Default port: COM3

set PORT=%1
if "%PORT%"=="" set PORT=COM3

python via_interactive_windows.py %PORT%
