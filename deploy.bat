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
    set OUTDIR=deploy\%%f
    
    md !OUTDIR!
    md !OUTDIR!\pythonprov
    md !OUTDIR!\rubyprov
    md !OUTDIR!\luaprov
    
    set LUA_BIN_DIR=debug
    set INVERSE=
    
    if /I "%%f" NEQ "debug" (
        set LUA_BIN_DIR=release
		set INVERSE=/v
    )
    
    REM Copy DbgScript host DLL/PDB.
    REM
    copy build\%%f\dbgscript.dll deploy\%%f\
    copy build\%%f\dbgscript.pdb deploy\%%f\
    
    REM ========================================================================
    REM Copy Python provider.
    REM
    copy build\src\pythonprov\%%f\pythonprov.dll deploy\%%f\pythonprov\
    copy build\src\pythonprov\%%f\pythonprov.pdb deploy\%%f\pythonprov\

    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib deploy\%%f\pythonprov\Lib\
    
    REM ========================================================================
    REM Copy Lua provider.
    REM
    copy build\src\luaprov\%%f\luaprov.dll deploy\%%f\luaprov\
    copy build\src\luaprov\%%f\luaprov.pdb deploy\%%f\luaprov\
    
    REM Copy the Lua DLLs
    REM
    copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.dll deploy\%%f\luaprov\
    copy deps\lua-%LUA_VER%\bin\!LUA_BIN_DIR!\lua-%LUA_VER%.pdb deploy\%%f\luaprov\
    
    REM ========================================================================
    REM Copy Ruby provider.
    REM
    copy build_rb_prov\%%f\rubyprov.dll deploy\%%f\rubyprov\
    copy build_rb_prov\%%f\rubyprov.pdb deploy\%%f\rubyprov\
    
    REM Copy over the Ruby standard library.
    REM
    xcopy /EYQ deps\ruby\runtime\lib deploy\%%f\rubyprov\Lib\
)
