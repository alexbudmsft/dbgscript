cur_thd = DbgScript.current_thread

stack = cur_thd.get_stack

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

