a = dbgscript.currentThread()
print(a.engineId, a.threadId)

b = dbgscript.getThreads()
for i = 1, #b do print (string.format(
    "Thread idx %d, id %d (0x%x), TEB: %x",
    b[i].engineId,
    b[i].threadId,
    b[i].threadId,
    b[i].teb)) end