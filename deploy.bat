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
    
    if not exist !OUTDIR! (
         md !OUTDIR!
    )
    
    if "%%f" == "debug" (
        set PYTHON_DLL=!PYTHON_DLL!_d
    )
    
    
    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib deploy\%%f\Lib\
    
    REM Copy the Python DLLs
    REM
    copy deps\python\DLLs\!PYTHON_DLL!.dll deploy\%%f\
    
    REM Copy all standard native modules
    REM
    for /F %%a in (%%f_pyd.txt) do copy %%a deploy\%%f\
    
    REM Delete the temp file.
    REM
    del %%f_pyd.txt
    
    REM Copy all PDBs. Actually, maybe not, for now. They're big, and do we
    REM really need them?
    REM
    REM copy deps\python\DLLs\*.pdb deploy\%%f\
    
    copy build\x64\%%f\dbgscript.dll deploy\%%f\
    copy build\x64\%%f\dbgscript.pdb deploy\%%f\
)
