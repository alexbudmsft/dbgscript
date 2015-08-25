car = DbgScript.create_typed_object('dummy1!Car', 0x00000f8a6cefdb0)

# Read ANSI string.
#
puts (car.f['name'].read_string())
puts (car.f['name'].read_string(1))
puts (car.f['name'].read_string(2))
puts (car.f['name'].read_string(-1))
puts (car.f['name'].read_string(100000))