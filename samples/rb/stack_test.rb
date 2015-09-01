thread = DbgScript.get_threads[0]
cur_thd = DbgScript.current_thread

stack = thread.get_stack
puts thread.teb.to_s(16)

for f in stack do
    locals = f.get_locals

    puts "Locals:"
    for v in locals
        puts \
            "Name: #{v.name}, "  \
            "Type: #{v.type}, "  \
            "Mod: #{v.module}, " \
            "Size: #{v.data_size}, " \
            "Addr: #{v.address.to_s(16)}"
    end

end

