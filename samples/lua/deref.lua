local worker = dbgscript.currentThread():currentFrame():getArgs()[1]
print(worker.name, worker.type, worker.address)
local b = worker.deref
print(b.name, b.type, b.address)
