a = dbgscript.create_typed_object('hkengtest!StressMgr', 0x00000050599be800)
print(a.name)
b = a.Runtime.Threads

print(b.name, b.type, b.size)

print (b[0].ThreadId.value)
print (len(a.Runtime.LogLocation))
print (a.Runtime.LogLocation.read_wide_string())