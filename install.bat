@echo off
setlocal EnableDelayedExpansion

REM This script should reside in the deployment directory beside the payload.
REM
set BASEDIR=%~dp0
set INSTALLDIR=%ProgramFiles%\DbgScript
if exist "%INSTALLDIR%" (
	set /P REPLACE="%INSTALLDIR% already exists. Replace? [y/N]: "
	if /I "!REPLACE!" == "y" (
		rd /q/s "%INSTALLDIR%"
		goto install
	) else (
		echo Exiting...
		exit /b
	)
)

:install
REM Copy over the bits.
REM
echo Copying files...
robocopy "%BASEDIR%\DbgScript" "%INSTALLDIR%" /MIR /njh /njs /ndl /nc /ns /nfl /np

if exist "%BASEDIR%\DbgScript\dbgscript-lockdown.dll" (
	set LCKSUFFIX=-lockdown
)

REM Setup registry.
REM
set BASEREGKEY=HKCU\Software\Microsoft\DbgScript
set PROVKEY=%BASEREGKEY%\Providers

echo Registering script providers (%PROVKEY%)...

reg add %PROVKEY% /f /t REG_SZ /v py /d "%INSTALLDIR%\pythonprov\pythonprov%LCKSUFFIX%.dll" > NUL
reg add %PROVKEY% /f /t REG_SZ /v rb /d "%INSTALLDIR%\rubyprov\rubyprov%LCKSUFFIX%.dll" > NUL
reg add %PROVKEY% /f /t REG_SZ /v lua /d "%INSTALLDIR%\luaprov\luaprov%LCKSUFFIX%.dll" > NUL

echo.
echo *********
echo All done^^! Use ".load %INSTALLDIR%\dbgscript%LCKSUFFIX%.dll" to load extension.
echo *********
echo.
echo NOTE: Ensure you have the VS 2015 and 2013 x64 Redists installed!
echo.
echo VS 2015: https://www.microsoft.com/en-us/download/details.aspx?id=48145
echo VS 2013: https://www.microsoft.com/en-us/download/details.aspx?id=40784
