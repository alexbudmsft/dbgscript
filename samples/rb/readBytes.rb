addr = ARGV[1].to_i(16)
count = 10
car = DbgScript.create_typed_object('dummy1!Car', addr)
x = car.read_bytes(count)
puts x.object_id, x.bytes

x = DbgScript.read_bytes(addr, count)
puts x.object_id, x.bytes