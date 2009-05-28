
rem ------------------------------------------------------------------------------
rem Tiny script to execute a single unit test suite.
rem 
rem Usage:
rem    SET  OUTDIR=<directory_for_test_results>
rem    SET  BINDIR=<directory_where_binaries_are_stored>
rem
rem    CALL RunSingleUnitTestSuite <name_of_test> <output_file>
rem
rem Post:
rem    FIRSTUTNA       - if the test wasn't found, receives the test name
rem    FIRSTUTFAILUR   - if the test failed, receives the test name
rem
rem ------------------------------------------------------------------------------
IF NOT EXIST %BINDIR%\%1\unit.exe (

   echo NOT AVAILABLE. Please rebuild this configuration
   echo Unable to find %BINDIR%\%1\unit.exe > %OUTDIR%%2
   SET FIRSTUTNA=%2
) ELSE (

   %BINDIR%\%1\unit.exe > %OUTDIR%%2
   IF errorlevel == 0 ( 
      echo SUCCESS
   ) ELSE (
      echo FAILURE, check output file: %2
      SET FIRSTUTFAILURE=%2
   )
)

echo.
echo.