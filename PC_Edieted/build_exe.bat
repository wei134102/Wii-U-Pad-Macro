@echo off
setlocal
cd /d "%~dp0"

rem CSR BlueSuite etc. set TCL_LIBRARY and break PyInstaller's Tk probe + frozen app.
set "TCL_LIBRARY="
set "TK_LIBRARY="

where py >nul 2>&1 && set "PY=py -3" || set "PY=python"

%PY% -m pip install "pyinstaller>=6.0"
if errorlevel 1 exit /b 1

%PY% -m PyInstaller --noconfirm --clean --windowed --onefile --name GamePadMacroEditor --collect-all tkinter editor.py
if errorlevel 1 exit /b 1

if not exist "dist\sd\wiiu\gamepad_macro" mkdir "dist\sd\wiiu\gamepad_macro"
xcopy /E /I /Y "..\sd\wiiu\gamepad_macro\*" "dist\sd\wiiu\gamepad_macro\" >nul

echo.
echo Done: dist\GamePadMacroEditor.exe
echo Copy dist\sd next to the exe if you move the executable alone.
endlocal
