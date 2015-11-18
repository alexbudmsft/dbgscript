@echo off
setlocal enabledelayedexpansion

echo Deploying dbgscript...

if exist deploy (
    rd /q/s deploy
    md deploy
)

md deploy\lockdown

set FLAVORS=Debug RelWithDebInfo

set LUA_VER=5.3.1

for %%f in (%FLAVORS%) do (
    set REG_DROP=deploy\%%f
    set LOCKDOWN_DROP=deploy\lockdown\%%f
	
	call :deploy_flavor %%f !REG_DROP!
	call :deploy_flavor %%f !LOCKDOWN_DROP! lockdown
)
goto :eof

REM Function to deploy a single flavor to a given output dir.
REM
:deploy_flavor
setlocal

set FLAV=%1
set OUTDIR=%2
set DBGSCRIPT_DIR=%OUTDIR%\DbgScript
if /I "%3" == "lockdown" (
	set LCKSUFFIX=-lockdown
)

echo Deploying %FLAV% to %OUTDIR%
echo --------------------------------------

md %DBGSCRIPT_DIR%

REM Don't bundle Python or Lua until they're locked down.
REM
if /I "%3" NEQ "lockdown" (
	md %DBGSCRIPT_DIR%\pythonprov
	md %DBGSCRIPT_DIR%\luaprov
) else (
    REM Copy lockdown notes only for the lockdown distro.
    REM
    copy lockdownnotes.md %OUTDIR%\
)
md %DBGSCRIPT_DIR%\rubyprov

REM Copy installation script.
REM
copy install.bat %OUTDIR%\

REM Copy release notes and license.
REM
copy releasenotes.md %OUTDIR%\
copy LICENSE.txt %OUTDIR%\

set LUA_BIN_DIR=debug

if /I "%FLAV%" NEQ "debug" (
	set LUA_BIN_DIR=release
)

REM Copy DbgScript host DLL/PDB.
REM
copy build\%FLAV%\dbgscript%LCKSUFFIX%.dll %DBGSCRIPT_DIR%\
copy build\%FLAV%\dbgscript%LCKSUFFIX%.pdb %DBGSCRIPT_DIR%\

REM Don't bundle Python or Lua until they're locked down.
REM
if /I "%3" NEQ "lockdown" (
	REM ========================================================================
	REM Copy Python provider.
	REM
	copy build\src\pythonprov\%FLAV%\pythonprov%LCKSUFFIX%.dll %DBGSCRIPT_DIR%\pythonprov\
	copy build\src\pythonprov\%FLAV%\pythonprov%LCKSUFFIX%.pdb %DBGSCRIPT_DIR%\pythonprov\

	REM Copy over the Python standard library.
	REM
	xcopy /EYQ deps\python\runtime\lib %DBGSCRIPT_DIR%\pythonprov\Lib\

	REM ========================================================================
	REM Copy Lua provider.
	REM
	copy build\src\luaprov\%FLAV%\luaprov%LCKSUFFIX%.dll %DBGSCRIPT_DIR%\luaprov\
	copy build\src\luaprov\%FLAV%\luaprov%LCKSUFFIX%.pdb %DBGSCRIPT_DIR%\luaprov\

	REM Copy the Lua DLLs
	REM
	copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.dll %DBGSCRIPT_DIR%\luaprov\
	copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.pdb %DBGSCRIPT_DIR%\luaprov\
)

REM ========================================================================
REM Copy Ruby provider.
REM
copy build_rb_prov\%FLAV%\rubyprov%LCKSUFFIX%.dll %DBGSCRIPT_DIR%\rubyprov\
copy build_rb_prov\%FLAV%\rubyprov%LCKSUFFIX%.pdb %DBGSCRIPT_DIR%\rubyprov\

REM Copy over the Ruby standard library.
REM
xcopy /EYQ deps\ruby\runtime\lib %DBGSCRIPT_DIR%\rubyprov\lib\

endlocal
