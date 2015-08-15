-- print (1)
-- a, err = io.read()
-- print(io.stdin, io.stdout, io.stderr)
-- print (a, err)

for x,y in pairs(package.loaded) do
    print (x, y)
end

print 'dbg script module:'
for x,y in pairs(dbgscript) do
    print (x, y)
end

print (dbgscript.teb())