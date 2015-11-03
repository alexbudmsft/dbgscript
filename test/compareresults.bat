@echo off
set TESTNAME=%1
set RESULTNAME=%TESTNAME%-result.txt
fc expected\%RESULTNAME% results\%RESULTNAME% > NUL
if errorlevel 1 (
	REM Append .fail suffix to ensure failed test runs again while still allowing
	REM inspection of content.
	REM
	move results\%RESULTNAME% results\%RESULTNAME%.fail > NUL
	echo %TESTNAME% FAIL (c.f. expected\%RESULTNAME% results\%RESULTNAME%.fail^)
) else (
	echo %TESTNAME% PASS
)
