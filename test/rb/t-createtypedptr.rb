require_relative 'utils'

car = get_car

addr = car['name'].address

# Obtain a pointer to the name buffer.
#
tp = DbgScript.create_typed_pointer('kernelbase!char', addr)

# Print the first few characters.
#
puts tp[0].value.chr
puts tp[1].value.chr
