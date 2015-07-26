import sys
from dbgscript import *
from pprint import pprint

#pprint (sys.modules)
#pprint (dir(dbgscript))
#pprint (vars(dbgscript))

#pprint (dir(Process))

threads = Process.get_threads()
cur_thd = Process.current_thread;

print ("Cur thread:", cur_thd, "Id:", cur_thd.thread_id)

pprint (cur_thd.get_stack())

#pprint (threads)
#print (dir(threads[0]))

firstThread = threads[0]
#secondThread = threads[1]

print (firstThread.engine_id, firstThread.thread_id, hex(firstThread.teb))

print (type(firstThread.teb)) # --> int

stack = firstThread.get_stack()
pprint (stack)

print (len(stack))

for i in range(len(stack)):
	print("Frame:", stack[i].frame_number, "Instruction Offset: ", hex(stack[i].instruction_offset))
	locals = stack[i].get_locals()
	for sym in locals:
		print ("Name: '{0}', Type: '{1}', Size: {2}, Addr: {3:#x}".
			format(sym.name, sym.type, sym.size, sym.address))

# del threads[0].thread_id # Fails: readonly attribute