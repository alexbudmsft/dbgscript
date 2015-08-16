a = DbgScript.create_typed_object('hkengtest!StressMgr', 0x00000050599be800)
puts a.name
b = a.Runtime.Threads

puts b.name, b.type, b.data_size

puts (b[0].ThreadId.value)
puts (a.Runtime.LogLocation.size)