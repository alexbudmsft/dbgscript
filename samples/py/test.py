import sys
from pprint import pprint

#pprint (sys.modules)
#pprint (dir(dbgscript))
#pprint (vars(dbgscript))

#pprint (dir(dbgscript))

threads = dbgscript.get_threads()
cur_thd = dbgscript.current_thread();

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

peb = None

for i in range(len(stack)):
	print("Frame:", stack[i].frame_number, "Instruction Offset: ", hex(stack[i].instruction_offset))
	locals = stack[i].get_locals()
	for sym in locals:
		if sym.name == 'LdrpDllActivateActivationContext_ActivationFrame':
			peb = sym
		print ("Name: '{0}', Type: '{1}', Size: {2}, Addr: {3:#x}".
			format(sym.name, sym.type, sym.size, sym.address))

print (peb)
# peb[3] => key must be unicode
field = peb['Format']
print ("Name: '{0}', Type: '{1}', Size: {2}, Addr: {3:#x}".
	format(field.name, field.type, field.size, field.address))
# del threads[0].thread_id # Fails: readonly attribute