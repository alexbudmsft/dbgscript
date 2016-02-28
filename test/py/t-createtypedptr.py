from utils import *

car = get_car()

addr = car['name'].address

# Obtain a pointer to the name buffer.
#
tp = dbgscript.create_typed_pointer('kernelbase!char', addr)

# Print the first few characters.
#
print (tp[0].value)
print (tp[1].value)
