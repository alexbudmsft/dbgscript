#p DbgScript.read_ptr(0x000000ac176eec48)
#t = DbgScript::Thread.new
#p t
#p t.engine_id
t = DbgScript.current_thread
#p t
#p t.engine_id, t.thread_id
#frame = t.current_frame
#p frame
#puts 'frame:', frame.frame_number, frame.instruction_offset
#puts 'hello'
#frame.frame_number = 4

stack = t.get_stack

stack.each do |frame|
    puts "#{frame.frame_number}: Instruction offset: 0x#{frame.instruction_offset.to_s(16)}"
    args = frame.get_args
    for a in args do
        puts a.name, a.type
    end
end
=begin
puts 0x000000ac176ee780.class
runtime = DbgScript.create_typed_object('hkengtest!UtRuntime', 0x000000ac176ee780)
ja = runtime['JoinAllStart']['Event']
puts ja.name, ja.type, ja.address, ja.size
#evt = ja['Event']
#puts evt
obj = DbgScript.create_typed_object('bool', 0xac176ee7e8)
p obj.name, obj.value, obj.type, obj.address, obj.size

for i in 0..runtime['CountThreads'].value
    thd = runtime['Threads'][i]
    puts i, thd.name, thd.address, thd.type, thd.size
end
=end
g = DbgScript.get_global('hkengtest!s_SbRingBuffer')
puts g.name, g.type, g.address
#frame.clone
#frame.dup
#line = STDIN.gets
#puts line

#puts 'aaa'

#for i in 0..5
#    puts i
#end