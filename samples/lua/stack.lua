a = dbgscript.currentThread()
-- print(a.engineId, a.threadId)

b = a:getStack()
for i = 1, #b do print (b[i].frameNumber, b[i].instructionOffset) end