import dbgscript
runtime = dbgscript.create_typed_object('hkengtest!UtRuntime', 0x000000ac176ee780)
ja = runtime['JoinAllStart']['Event']
print ( ja.name, ja.type, ja.address, ja.size )
obj = dbgscript.create_typed_object('bool', 0xac176ee7e8)
print ( obj.name, obj.value, obj.type, obj.address, obj.size )

for i in range(runtime['CountThreads'].value):
    thd = runtime['Threads'][i]
    print ( i, thd.name, thd.address, thd.type, thd.size )

