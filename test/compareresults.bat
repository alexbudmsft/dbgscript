@echo off
set RESULTNAME=%1
fc results\%RESULTNAME% expected\%RESULTNAME% > NUL
if errorlevel 1 (
	REM Append .fail suffix to ensure failed test runs again while still allowing
	REM inspection of content.
	REM
	move results\%RESULTNAME% results\%RESULTNAME%.fail > NUL
	echo %RESULTNAME% FAIL (see results\%RESULTNAME%.fail^)
) else (
	echo %RESULTNAME% PASS
)
