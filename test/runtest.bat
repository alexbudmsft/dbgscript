@echo off

set TESTNAME=%1
set DMPNAME=%2

REM Run the test against the dump.
REM
cdb -z %DMPNAME% -cf %TESTNAME%.txt > NUL

REM Compare results.
REM
call compareresults.bat %TESTNAME%-result.txt
