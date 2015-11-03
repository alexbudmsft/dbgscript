t = DbgScript.current_thread
f = t.current_frame
locals = f.get_locals

# Get the 'car' local.
#
car = locals.find {|t| t.name == 'car'}

# Read ANSI string.
#
puts (car['name'].read_string)    # until NUL
puts (car['name'].read_string(1))   # first char
puts (car['name'].read_string(2))   # first two chars
puts (car['name'].read_string(-1))  # until NUL

# Read Wide string.
#
puts (car['wide_name'].read_wide_string)    # until NUL
puts (car['wide_name'].read_wide_string(1))   # first char
puts (car['wide_name'].read_wide_string(2))   # first two chars
puts (car['wide_name'].read_wide_string(-1))  # until NUL