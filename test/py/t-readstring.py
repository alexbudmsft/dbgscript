t = dbgscript.current_thread()
f = t.current_frame
locals = f.get_locals()

# Get the 'car' local. Throws StopIteration if not found.
#
car = next(t for t in locals if t.name == 'car')

# Read ANSI string.
#
print (car['name'].read_string())    # until NUL
print (car['name'].read_string(1))   # first char
print (car['name'].read_string(2))   # first two chars
print (car['name'].read_string(-1))  # until NUL

# Read Wide string.
#
print (car['wide_name'].read_wide_string())    # until NUL
print (car['wide_name'].read_wide_string(1))   # first char
print (car['wide_name'].read_wide_string(2))   # first two chars
print (car['wide_name'].read_wide_string(-1))  # until NUL