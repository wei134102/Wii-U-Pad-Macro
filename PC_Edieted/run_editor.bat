@echo off
cd /d "%~dp0"
rem CSR BlueSuite and similar tools set TCL_LIBRARY and break Python's Tk.
set "TCL_LIBRARY="
set "TK_LIBRARY="

where py >nul 2>&1 && (
  py -3 editor.py
  if not errorlevel 1 goto :eof
)
python editor.py
if errorlevel 1 pause
