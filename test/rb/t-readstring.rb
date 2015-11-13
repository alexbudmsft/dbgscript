require_relative 'utils'

t = DbgScript.current_thread
f = t.current_frame
locals = f.get_locals

# Get the 'car' local.
#
car = locals.find {|t| t.name == 'car'}

# Read ANSI string.
#
puts car['name'].read_string      # until NUL
puts car['name'].read_string(1)   # first char
puts car['name'].read_string(2)   # first two chars
puts car['name'].read_string(-1)  # until NUL

# Longer than the real length. Expect full string to be read without any embedded
# NULs.
#
puts car['name'].read_string(1000)

# Exercise DbgScript#read_string.
#
puts DbgScript.read_string(car['name'].address)
puts DbgScript.read_string(car['name'].address, 1)
puts DbgScript.read_string(car['name'].address, 5)
puts DbgScript.read_string(car['name'].address, 500)
puts DbgScript.read_string(car['name'].address, -1)

# Negative tests.
#
negative_test(ArgumentError) {
  puts car['name'].read_string(100000000)  # too big
}
negative_test(ArgumentError) {
  puts car['name'].read_string(0)  # 0 invalid
}
negative_test(ArgumentError) {
  puts DbgScript.read_string(car['name'].address, 0)
}
negative_test(ArgumentError) {
  puts DbgScript.read_string(car['name'].address, 10000000)
}

# Read Wide string.
#
puts car['wide_name'].read_wide_string      # until NUL
puts car['wide_name'].read_wide_string(1)   # first char
puts car['wide_name'].read_wide_string(2)   # first two chars
puts car['wide_name'].read_wide_string(-1)  # until NUL

# Longer than the real length. Expect full string to be read without any embedded
# NULs.
#
puts car['wide_name'].read_wide_string(1000)

# Negative test.
#
negative_test(ArgumentError) {
  puts car['wide_name'].read_wide_string(0)
}
negative_test(ArgumentError) {
  puts car['wide_name'].read_wide_string(1000000)
}

# Exercise DbgScript#read_wide_string.
#
puts DbgScript.read_wide_string(car['wide_name'].address)
puts DbgScript.read_wide_string(car['wide_name'].address, 1)
puts DbgScript.read_wide_string(car['wide_name'].address, 5)
puts DbgScript.read_wide_string(car['wide_name'].address, 500)
puts DbgScript.read_wide_string(car['wide_name'].address, -1)
