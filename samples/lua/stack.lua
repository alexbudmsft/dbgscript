a = dbgscript.currentThread()
-- print(a.engineId, a.threadId)

b = a:getStack()

a=nil

for i = 1, #b do
    print (b[i].frameNumber, b[i].instructionOffset)
    local loc = b[i]:getLocals()
    for j = 1, #loc do
        print (loc[j])
    end
end
