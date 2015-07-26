from dbgscript import *

cur_thd = Process.current_thread

print ("Engine Thd ID:", cur_thd.engine_id, "Sys Id:", cur_thd.thread_id)
stack = cur_thd.get_stack()

locals = stack[5].get_variables()

for v in locals:
	print ("Name:", v.name, "Size:", v.size)