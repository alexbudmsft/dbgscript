import sys
from dbgscript import *
from pprint import pprint

#pprint (sys.modules)
#pprint (dir(dbgscript))
#pprint (vars(dbgscript))

#pprint (dir(Process))

threads = Process.get_threads()

#pprint (threads)
#print (dir(threads[0]))

firstThread = threads[0]

print (firstThread.engine_id, firstThread.thread_id, hex(firstThread.teb))

print (type(firstThread.teb)) # --> int
