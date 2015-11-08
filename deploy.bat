@echo off
setlocal enabledelayedexpansion

echo Deploying dbgscript...

rd /q/s deploy
md deploy

set FLAVORS=Debug RelWithDebInfo

set LUA_VER=5.3.1

for %%f in (%FLAVORS%) do (
    echo Deploying %%f
    echo ---------------------
    set OUTDIR=deploy\%%f\DbgScript
    
    md !OUTDIR!
    md !OUTDIR!\pythonprov
    md !OUTDIR!\rubyprov
    md !OUTDIR!\luaprov
    
    REM Copy installation script.
    REM
    copy install.bat deploy\%%f\
    
    REM Copy release notes.
    REM
    copy releasenotes.md deploy\%%f\
    
    set LUA_BIN_DIR=debug
    
    if /I "%%f" NEQ "debug" (
        set LUA_BIN_DIR=release
    )
    
    REM Copy DbgScript host DLL/PDB.
    REM
    copy build\%%f\dbgscript.dll !OUTDIR!\
    copy build\%%f\dbgscript.pdb !OUTDIR!\
    
    REM ========================================================================
    REM Copy Python provider.
    REM
    copy build\src\pythonprov\%%f\pythonprov.dll !OUTDIR!\pythonprov\
    copy build\src\pythonprov\%%f\pythonprov.pdb !OUTDIR!\pythonprov\

    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib !OUTDIR!\pythonprov\Lib\
    
    REM ========================================================================
    REM Copy Lua provider.
    REM
    copy build\src\luaprov\%%f\luaprov.dll !OUTDIR!\luaprov\
    copy build\src\luaprov\%%f\luaprov.pdb !OUTDIR!\luaprov\
    
    REM Copy the Lua DLLs
    REM
    copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.dll !OUTDIR!\luaprov\
    copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.pdb !OUTDIR!\luaprov\
    
    REM ========================================================================
    REM Copy Ruby provider.
    REM
    copy build_rb_prov\%%f\rubyprov.dll !OUTDIR!\rubyprov\
    copy build_rb_prov\%%f\rubyprov.pdb !OUTDIR!\rubyprov\
    
    REM Copy over the Ruby standard library.
    REM
    xcopy /EYQ deps\ruby\runtime\lib !OUTDIR!\rubyprov\lib\
)
