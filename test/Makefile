CL=cl
DMPNAME=dummy.dmp
all: setup runtests

setup: $(DMPNAME) results

results:
	md results

# Build the dummy app.
#
dummy.exe: dummy.cpp
	@echo Compiling dummy application...
	$(CL) /nologo /Zi /WX /W4 $** /link /release > NUL

# Produce a dump.
#
$(DMPNAME): dummy.exe
	@echo Making dump...
	if exist $(DMPNAME) del $(DMPNAME)
	cdb -cf makedmpcmds.txt dummy.exe > NUL

runtests: \
	results\t-simple-result.txt \
	results\t-readstring-result.txt

results\t-simple-result.txt: t-simple.txt py\t-simple.py
	call runtest.bat t-simple $(DMPNAME)

results\t-readstring-result.txt: t-readstring.txt py\t-readstring.py
	call runtest.bat t-readstring $(DMPNAME)

clean:
	-del *.obj *.pdb *.exe *.dmp *.ilk
	-rd /q/s results