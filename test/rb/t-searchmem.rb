require_relative 'utils'

car = get_car

# Positive cases. Can't print the result because the address may change
# from run to run.
#
DbgScript.search_memory(car['name'].address-16, 100, 'FooCar', 1)
DbgScript.search_memory(car['name'].address-16, 100, 'FooCar', 2)

# Negative cases.
#

# 4 is not a multiple of the pattern length.
#
negative_test(ArgumentError) {
  DbgScript.search_memory(car['name'].address-16, 100, 'FooCar', 4)
}

# Try a non-existent pattern.
#
negative_test(KeyError) {
  DbgScript.search_memory(car['name'].address-16, 100, 'AbcDefAb', 4)
}

# 3 is a multiple of the pat. len, but the pattern won't be found on a
# 3 byte granularity.
#
negative_test(KeyError) {
  DbgScript.search_memory(car['name'].address-16, 100, 'FooCar', 3)
}
