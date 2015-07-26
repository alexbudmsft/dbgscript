from dbgscript import *

cur_thd = Process.current_thread

print ("Engine Thd ID:", cur_thd.engine_id, "Sys Id:", cur_thd.thread_id)
stack = cur_thd.get_stack()

frameidx = 0xd

locals = stack[frameidx].get_locals()
args = stack[frameidx].get_args()

print ("Locals:")
for v in locals:
	print ("Name:", v.name, "Size:", v.size)
	
print ("Args:")
for v in args:
	print ("Name:", v.name, "Size:", v.size)	