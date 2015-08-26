car = dbgscript.createTypedObject('dummy1!Car', 0x000005a9341fb10)

-- Read ANSI string:
--
print (car:f('f'):f('name'):readString())
print (car:f('f'):f('name'):readString(1))
print (car:f('f'):f('name'):readString(2))
print (car:f('f'):f('name'):readString(-1))
--print (car:f('f'):f('name'):readString(100000)) --> error

print (car:f('f').wide_name:readWideString())
print (car:f('f').wide_name:readWideString(1))
print (car:f('f').wide_name:readWideString(2))
--print (car:f('f')['wide_name']:readWideString(0)) --> error
print (car:f('f').wide_name:readWideString(-1))
