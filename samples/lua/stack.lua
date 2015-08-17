a = dbgscript.currentThread()
-- print(a.engineId, a.threadId)

b = a:getStack()

a=nil -- Lose ref to thread, allowing GC to clean it up.

for i = 1, #b do
    print (b[i].frameNumber, b[i].instructionOffset)
    
    print "Locals"
    print "---------------------"
    local loc = b[i]:getLocals()
    for j = 1, #loc do
        print(string.format('Name: %s, Addr: 0x%x',
            loc[j].name, loc[j].address))
    end
    
    print "\nArgs"
    print "---------------------"
    local loc = b[i]:getArgs()
    for j = 1, #loc do
        print (loc[j].name)
    end
end
