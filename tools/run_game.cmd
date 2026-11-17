@echo off
rem ==============================================================
rem run_game.cmd - launch the locally built Orbiter without the
rem Windows "Open File - Security Warning" dialog.
rem
rem That dialog is shown by ShellExecute for unsigned executables
rem (e.g. when double-clicking Orbiter.exe in Explorer). Launching
rem the executable from a command script uses CreateProcess instead
rem and therefore does not trigger the warning.
rem
rem Usage:
rem   tools\run_game.cmd                     (Launchpad)
rem   tools\run_game.cmd --scenariox="Delta-glider\DG and DG-S" --maxsystime=30
rem ==============================================================
setlocal

set "GAME=%~dp0..\install64\Orbiter"
if not exist "%GAME%\Orbiter.exe" (
    echo [ERROR] Orbiter.exe not found: %GAME%\Orbiter.exe
    exit /b 1
)

cd /d "%GAME%"
Orbiter.exe %*
exit /b %ERRORLEVEL%
