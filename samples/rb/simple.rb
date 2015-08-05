#p DbgScript.read_ptr(0x000000ac176eec48)
#t = DbgScript::Thread.new
#p t
#p t.engine_id
t = DbgScript.current_thread
p t
p t.engine_id, t.thread_id

#line = STDIN.gets
#puts line

#exit!
#p 'aaaaa'
#puts $:
#require 'debug'
#require 'base64'

puts 'aaa'
#begin
#    require 'aaa'
#rescue LoadError
#    puts 'got an error'
#end


#puts 'aaa'

#for i in 0..5
#    puts i
#end