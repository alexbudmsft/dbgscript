car = dbgscript.create_typed_object('dummy1!Car', 0x00000f8a6cefdb0)

# Read ANSI string.
#
print (car.f['name'].read_string())
print (car.f['name'].read_string(1))
print (car.f['name'].read_string(2))
print (car.f['name'].read_string(-1))