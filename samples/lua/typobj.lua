a = dbgscript.createTypedObject('hkengtest!StressMgr', 0x00000050599be800)
print(a.name)
b = a.CompletionInfo
--c = a['CompletionInfo']
--d = a:f('CompletionInfo')
print (a:field('CompletionInfo'))
print (a:f('CompletionInfo'))

print(b.name, b.type, b.size)
