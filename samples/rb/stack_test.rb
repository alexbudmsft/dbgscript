cur_thd = DbgScript.current_thread()

stack = cur_thd.get_stack()

frameidx = 2

locals = stack[frameidx].get_locals()
args = stack[frameidx].get_args()

puts "Locals:"
for v in locals
	puts "Name: #{v.name}, Size: #{v.data_size}"
end

puts "Args:"
for v in args
	puts "Name: #{v.name}, Size: #{v.data_size}"
end
