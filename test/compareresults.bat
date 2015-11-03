@echo off
set TESTNAME=%1
fc results\%TESTNAME% expected\%TESTNAME% > NUL
if errorlevel 1 (
	move results\%TESTNAME% results\%TESTNAME%.fail > NUL
	echo %TESTNAME% FAIL (see results\%TESTNAME%.fail^)
) else (
	echo %TESTNAME% PASS
)
