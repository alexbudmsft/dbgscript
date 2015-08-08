@echo off
setlocal enabledelayedexpansion

echo Deploying dbgscript...

rd /q/s deploy
md deploy

set FLAVORS=Debug RelWithDebInfo

set PYDLISTFILE=pydlist.txt
dir /s/b deps\python\DLLs\*.pyd > %PYDLISTFILE%

for %%f in (%FLAVORS%) do (
    echo Deploying %%f
    echo ---------------------
    set OUTDIR=deploy\%%f
    set PYTHON_DLL=python36
    
    md !OUTDIR!
    md !OUTDIR!\pythonprov
    md !OUTDIR!\rubyprov
    
    set INVERSE=
    if /I "%%f" == "debug" (
        set PYTHON_DLL=!PYTHON_DLL!_d
    ) else (
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
    
    REM Copy the Python DLLs
    REM
    copy deps\python\DLLs\!PYTHON_DLL!.dll deploy\%%f\pythonprov\
    copy deps\python\DLLs\!PYTHON_DLL!.pdb deploy\%%f\pythonprov\
    
    REM Copy all standard native modules
    REM
    for /F %%a in ('findstr !INVERSE! _d %PYDLISTFILE%') do copy %%a deploy\%%f\pythonprov\
    
    REM ========================================================================
    REM Copy Ruby provider.
    REM
    copy build_rb_prov\%%f\rubyprov.dll deploy\%%f\rubyprov\
    copy build_rb_prov\%%f\rubyprov.pdb deploy\%%f\rubyprov\
    
    REM Copy over the Ruby standard library.
    REM
    xcopy /EYQ deps\ruby\runtime\lib deploy\%%f\rubyprov\Lib\
)
    
REM Delete the temp file.
REM
del %PYDLISTFILE%