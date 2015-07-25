@echo off
echo Deploying...
if not exist deploy (
	md deploy
)
xcopy /EYQ deps\python\runtime\lib deploy\Lib\
copy deps\python\lib\*.dll deploy\
copy deps\python\lib\*.pdb deploy\
copy build\x64\debug\dbgscript.dll deploy\
copy build\x64\debug\dbgscript.pdb deploy\
