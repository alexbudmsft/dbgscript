import sys;
import dbgscript
from pprint import pprint

#pprint (sys.modules)
#pprint (dir(dbgscript))
#pprint (vars(dbgscript))

#pprint (dir(dbgscript.Thread))

print (hex(dbgscript.Thread.teb))

print (type(dbgscript.Thread.teb)) # --> int
