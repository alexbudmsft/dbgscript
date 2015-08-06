#p DbgScript.read_ptr(0x000000ac176eec48)
#t = DbgScript::Thread.new
#p t
#p t.engine_id
t = DbgScript.current_thread
p t
p t.engine_id, t.thread_id
frame = t.current_frame
p frame

stack = t.get_stack
p stack

#frame.clone
#frame.dup
#line = STDIN.gets
#puts line

#puts 'aaa'

#for i in 0..5
#    puts i
#end