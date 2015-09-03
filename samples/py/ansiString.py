car = dbgscript.create_typed_object('dummy1!Car', 0x00000eb64f1fbf0)

# Read ANSI string.
#
print (car.f['name'].read_string())
print (car.f['name'].read_string(1))
print (car.f['name'].read_string(2))
print (car.f['name'].read_string(-1))
#print (car.f['name'].read_string(100000)) # error: count too large.
#print (car.f['name'].read_string(0)) # error: count must be positive.