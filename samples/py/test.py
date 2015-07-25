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
secondThread = threads[1]

print (firstThread.engine_id, firstThread.thread_id, hex(firstThread.teb))

print (type(firstThread.teb)) # --> int

stack = secondThread.get_stack()
pprint (stack)

print (len(stack))

for i in range(len(stack)):
	print(stack[i].frame_number, "Instruction Offset: ", hex(stack[i].instruction_offset))

# del threads[0].thread_id # Fails: readonly attribute