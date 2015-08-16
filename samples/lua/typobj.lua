a = dbgscript.createTypedObject('hkengtest!StressMgr', 0x00000050599be800)
print(a.name)
b = a.Runtime.Threads
--c = a['CompletionInfo']
--d = a:f('CompletionInfo')
--print (a:field('CompletionInfo'))
--print (a:f('CompletionInfo'))

print(b.name, b.type, b.size)

print (b[0].ThreadId.value)
print (#a.Runtime.LogLocation)

--for k, v in pairs(arg) do print (k, v) end
for i = 0, #arg do print(arg[i]) end

print (#arg)