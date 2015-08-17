cur_thd = dbgscript.current_thread()

print ("Engine Thd ID:", cur_thd.engine_id, "Sys Id:", cur_thd.thread_id)

stack = cur_thd.get_stack()

frame_idx = 2

locals = stack[frame_idx].get_locals()
args = stack[frame_idx].get_args()

print ("Locals:")
for v in locals:
	print ("Name:", v.name, "Size:", v.size, "Type:", v.type, "Module:",
        v.module, "Addr:", v.address)
	
print ("Args:")
for v in args:
	print ("Name:", v.name, "Size:", v.size)