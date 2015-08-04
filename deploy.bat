@echo off
setlocal enabledelayedexpansion

echo Deploying dbgscript...

rd /q/s deploy
md deploy

set FLAVORS=debug release

dir /s/b deps\python\DLLs\*.pyd | findstr _d > debug_pyd.txt
dir /s/b deps\python\DLLs\*.pyd | findstr /v _d > release_pyd.txt

for %%f in (%FLAVORS%) do (
    echo Deploying %%f
    echo ---------------------
    set OUTDIR=deploy\%%f
    set PYTHON_DLL=python36
    
    md !OUTDIR!
    md !OUTDIR!\pythonprov
    md !OUTDIR!\rubyprov
    
    if "%%f" == "debug" (
        set PYTHON_DLL=!PYTHON_DLL!_d
    )

    REM Copy DbgScript host DLL/PDB.
    REM
    copy build\x64\%%f\dbgscript.dll deploy\%%f\
    copy build\x64\%%f\dbgscript.pdb deploy\%%f\
    
    REM ========================================================================
    REM Copy Python provider.
    REM
    copy build\x64\%%f\pythonprov.dll deploy\%%f\pythonprov\
    copy build\x64\%%f\pythonprov.pdb deploy\%%f\pythonprov\

    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib deploy\%%f\pythonprov\Lib\
    
    REM Copy the Python DLLs
    REM
    copy deps\python\DLLs\!PYTHON_DLL!.dll deploy\%%f\pythonprov\
    copy deps\python\DLLs\!PYTHON_DLL!.pdb deploy\%%f\pythonprov\
    
    REM Copy all standard native modules
    REM
    for /F %%a in (%%f_pyd.txt) do copy %%a deploy\%%f\pythonprov\
    
    REM Delete the temp file.
    REM
    del %%f_pyd.txt
    
    REM ========================================================================
    REM Copy Ruby provider.
    REM
    copy build\x64\%%f\rubyprov.dll deploy\%%f\rubyprov\
    copy build\x64\%%f\rubyprov.pdb deploy\%%f\rubyprov\
    
    REM Copy over the Ruby standard library.
    REM
    xcopy /EYQ deps\ruby\runtime\lib deploy\%%f\rubyprov\Lib\
    
    REM Copy the Ruby DLLs
    REM
    copy deps\ruby\lib\x64-msvcr120-ruby220.dll deploy\%%f\rubyprov\
    copy deps\ruby\lib\x64-msvcr120-ruby220.pdb deploy\%%f\rubyprov\
)
