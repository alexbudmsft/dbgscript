a = dbgscript.createTypedObject('hkengtest!StressMgr', 0x00000050599be800)
print(a.name)
b = a.CompletionInfo
print(b.name, b.type, b.size)
