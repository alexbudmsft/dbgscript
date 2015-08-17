a = dbgscript.currentThread()
print(a.engineId, a.threadId)

b = dbgscript.getThreads()
for i = 1, #b do print (b[i]) end