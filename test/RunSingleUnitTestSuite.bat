rem Alexander Gessler, 12:30:08


if exist %BINDIR%\%1\UnitTest.exe goto test1

echo NOT AVAILABLE. Please rebuild this configuration
echo Unable to find %BINDIR%\%1\UnitTest.exe > %OUTDIR%%2
SET FIRSTUTNA=%2
goto end:

:test1
%BINDIR%\%1\UnitTest.exe > %OUTDIR%%2
if     errorlevel == 0 goto succ

echo FAILURE, check output file: %2
SET FIRSTUTFAILURE=%2
goto end

:succ
echo SUCCESS

:end
echo.
echo.