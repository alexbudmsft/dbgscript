# Test runner makefile.
# *********************
#
# To add a new test:
# ------------------
#
# 1) Create a script driving your test: t-<test_name>.txt. Create per-language
#    scripts in the corresponding subdirs. Ideally have a test in all 3
#    languages. See existing tests for examples.
#
#   a) Have your debugger script .logopen results\t-<test_name>-result.txt
#   b) Invoke language-specific scripts via !runscript -l <lang> .\<lang>\t-<test_name>.<lang>
#      See existing ones for examples.
#
# 2) Add a new Makefile target for your test, listing its dependencies.
# 3) Add your target under the coretests target.
# 4) Run runtests.bat to run them.
# 5) If generated output is as expected, copy the results file to expected\,
#    remove the .fail extension and check it in along with your other changes.
#
# Note: Tests execute incrementally. If you want to re-run a test, delete its
# results file under the results\ temp dir.
#

CL=cl
DMPNAME=dummy.dmp
all: setup coretests

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

# Primary tests. May be run in all flavors. Add new tests here.
#
coretests: \
	results\t-simple-result.txt \
	results\t-readstring-result.txt \
	results\t-createtypedptr-result.txt \
	results\t-gettypesize-result.txt \
	results\t-searchmem-result.txt \

# Lockdown tests. Run *only* if lockdown build is installed.
#
lockdown: \
	results\t-lockdown-result.txt

results\t-simple-result.txt: t-simple.txt py\t-simple.py
	call runtest.bat t-simple $(DMPNAME)

results\t-readstring-result.txt: \
	t-readstring.txt \
	py\t-readstring.py \
	rb\t-readstring.rb \
	lua\t-readstring.lua
	call runtest.bat t-readstring $(DMPNAME)

results\t-gettypesize-result.txt: \
	t-gettypesize.txt \
	py\t-gettypesize.py \
	rb\t-gettypesize.rb \
	lua\t-gettypesize.lua
	call runtest.bat t-gettypesize $(DMPNAME)

results\t-createtypedptr-result.txt: \
	t-createtypedptr.txt \
	rb\t-createtypedptr.rb
	call runtest.bat t-createtypedptr $(DMPNAME)

results\t-searchmem-result.txt: \
	t-searchmem.txt \
	py\t-searchmem.py \
	rb\t-searchmem.rb
	call runtest.bat t-searchmem $(DMPNAME)

results\t-lockdown-result.txt: t-lockdown.txt rb\t-lockdown.rb
	call runtest.bat t-lockdown $(DMPNAME)

clean:
	-del *.obj *.pdb *.exe *.dmp *.ilk
	-rd /q/s results