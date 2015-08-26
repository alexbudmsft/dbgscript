car = dbgscript.create_typed_object('dummy1!Car', 0x000005a9341fb10)

# Read ANSI string.
#
print (car.f['name'].read_string())
print (car.f['name'].read_string(1))
print (car.f['name'].read_string(2))
print (car.f['name'].read_string(-1))
#print (car.f['name'].read_string(100000))
#print (car.f['name'].read_string(0))