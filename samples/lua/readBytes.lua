function printBytes(x)
    for i = 1, x:len() do
        print (x:byte(i))
    end
end

addr = tonumber(arg[1], 16)
count = 10

car = dbgscript.createTypedObject('dummy1!Car', addr)
x = car:readBytes(count)
printBytes(x)

x = dbgscript.readBytes(addr, count)
printBytes(x)
