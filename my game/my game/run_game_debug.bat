@echo off
"%~dp0game.exe" > "%~dp0run_stdout3.txt" 2> "%~dp0run_stderr3.txt"
echo ExitCode=%ERRORLEVEL% >> "%~dp0run_stdout3.txt"
pause
