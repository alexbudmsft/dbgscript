CL=cl
DMPNAME=dummy.dmp
all: setup runtests

setup: $(DMPNAME) results

results:
	md results

# Build the dummy app.
#
dummy.exe: dummy.cpp
	$(CL) /nologo /Zi /WX /W4 $** /link /release

# Produce a dump.
#
$(DMPNAME): dummy.exe
	cdb -cf makedmpcmds.txt dummy.exe

runtests: results\t-simple-result.txt

results\t-simple-result.txt: t-simple.txt py\t-simple.py
	call runtest.bat t-simple $(DMPNAME)
	
clean:
	-del *.obj *.pdb *.exe *.dmp *.ilk
	-rd /q/s results