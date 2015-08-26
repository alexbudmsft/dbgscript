car = DbgScript.create_typed_object('dummy1!Car', 0x000005a9341fb10)

# Read ANSI string.
#
puts (car.f['name'].read_string())
puts (car.f['name'].read_string(1))
puts (car.f['name'].read_string(2))
puts (car.f['name'].read_string(-1))
#puts (car.f['name'].read_string(100000)) -> error

puts (car.f['wide_name'].read_wide_string())
puts (car.f['wide_name'].read_wide_string(1))
puts (car.f['wide_name'].read_wide_string(2))
#puts (car.f['wide_name'].read_wide_string(0)) -> error
puts (car.f['wide_name'].read_wide_string(-1))
