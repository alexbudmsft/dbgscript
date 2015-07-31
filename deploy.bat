@echo off
setlocal enabledelayedexpansion

echo Deploying dbgscript...

if not exist deploy (
	md deploy
)

set FLAVORS=debug release

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
    copy deps\python\lib\!PYTHON_DLL!.dll deploy\%%f\
    copy deps\python\lib\!PYTHON_DLL!.pdb deploy\%%f\
    
    copy build\x64\%%f\dbgscript.dll deploy\%%f\
    copy build\x64\%%f\dbgscript.pdb deploy\%%f\
)
