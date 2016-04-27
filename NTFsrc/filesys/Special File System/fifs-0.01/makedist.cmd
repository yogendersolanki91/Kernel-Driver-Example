@echo off

setlocal

set VER=0.01
set APP=fifs

set AVER=%APP%-%VER%
set DIR=%TEMP%\%AVER%-zip

rd /s %DIR%
mkdir %DIR%
pushd %DIR%
if errorlevel 1 goto :EOF
cvs co %APP%
ren %APP% %AVER%
if errorlevel 1 goto :EOF
zip -rX %AVER%.zip %AVER% -x */CVS/* -x */scripts/*

pause

cvs co frontrpc
cd frontrpc
nmake dist
nmake dist NODEBUG=1
cd ..

cd %AVER%
nmake all
nmake all NODEBUG=1
cd ..
zip -rX %AVER%-bin-debug.zip %AVER%\Debug
zip -rX %AVER%-bin.zip %AVER%\Release

popd
dir %DIR%

endlocal
