car = DbgScript.create_typed_object('dummy1!Car', ARGV[1].to_i(16))
x = car.read_bytes(10)
puts x.bytes