from utils import *

car = get_car()

# Positive cases. Can't print the result because the address may change
# from run to run.
#
dbgscript.search_memory(car['name'].address-16, 100, b'FooCar', 1)
dbgscript.search_memory(car['name'].address-16, 100, b'FooCar', 2)

# Negative cases.
#

# 4 is not a multiple of the pattern length.
#
try:
  print (dbgscript.search_memory(car['name'].address-16, 100, b'FooCar', 4))
except ValueError:
  print('Swallowed ValueError')

# Try a non-existent pattern.
#
try:
  print (dbgscript.search_memory(car['name'].address-16, 100, b'AbcDefAb', 4))
except LookupError:
  print('Swallowed LookupError')

# 3 is a multiple of the pat. len, but the pattern won't be found on a
# 3 byte granularity.
#
try:
  print (dbgscript.search_memory(car['name'].address-16, 100, b'FooCar', 3))
except LookupError:
  print('Swallowed LookupError')
