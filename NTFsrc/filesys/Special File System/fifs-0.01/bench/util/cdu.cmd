@echo off
if "%1" == "" goto :usage
grep du -A 2 %1 | grep -A 2 Running | grep -A 2 tail | grep -v "<<" | grep -v "\-\-\-"
goto :eof
:usage
cdu filename
