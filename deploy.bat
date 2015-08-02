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
    
    if "%%f" == "debug" (
        set PYTHON_DLL=!PYTHON_DLL!_d
    )

    REM Copy DbgScript host DLL/PDB.
    REM
    copy build\x64\%%f\dbgscript.dll deploy\%%f\
    copy build\x64\%%f\dbgscript.pdb deploy\%%f\
    
    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib deploy\%%f\pythonprov\Lib\
    
    REM Copy the Python DLLs
    REM
    copy deps\python\DLLs\!PYTHON_DLL!.dll deploy\%%f\pythonprov\
    
    REM Copy all standard native modules
    REM
    for /F %%a in (%%f_pyd.txt) do copy %%a deploy\%%f\pythonprov\
    
    REM Delete the temp file.
    REM
    del %%f_pyd.txt
    
    copy build\x64\%%f\pythonprov.dll deploy\%%f\pythonprov\
    copy build\x64\%%f\pythonprov.pdb deploy\%%f\pythonprov\
)
