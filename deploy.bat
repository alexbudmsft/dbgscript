@echo off
setlocal enabledelayedexpansion

echo Deploying...

if not exist deploy (
	md deploy
)

set FLAVORS=debug release

for %%f in (%FLAVORS%) do (
    echo Deploying %%f
    echo ---------------------
    set OUTDIR=deploy\%%f
    
    if not exist !OUTDIR! (
         md !OUTDIR!
    )
    
    REM Copy over the Python standard library.
    REM
    xcopy /EYQ deps\python\runtime\lib deploy\%%f\Lib\
    
    REM Copy the Python DLLs
    REM
    copy deps\python\lib\*.dll deploy\%%f\
    copy deps\python\lib\*.pdb deploy\%%f\
    
    copy build\x64\%%f\dbgscript.dll deploy\%%f\
    copy build\x64\%%f\dbgscript.pdb deploy\%%f\
)
