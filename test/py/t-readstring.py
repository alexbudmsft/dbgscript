t = dbgscript.current_thread()
f = t.current_frame

# 'locals' is a Python built-in function.
#
f_locals = f.get_locals()

# Get the 'car' local. Throws StopIteration if not found.
#
car = next(t for t in f_locals if t.name == 'car')

# Read ANSI string.
#
print (car['name'].read_string())    # until NUL
print (car['name'].read_string(1))   # first char
print (car['name'].read_string(2))   # first two chars
print (car['name'].read_string(-1))  # until NUL

# Exercise dbgscript.read_string.
#
print (dbgscript.read_string(car['name'].address))
print (dbgscript.read_string(car['name'].address, 1))
print (dbgscript.read_string(car['name'].address, 5))
print (dbgscript.read_string(car['name'].address, 500))
print (dbgscript.read_string(car['name'].address, -1))

# Read Wide string.
#
print (car['wide_name'].read_wide_string())    # until NUL
print (car['wide_name'].read_wide_string(1))   # first char
print (car['wide_name'].read_wide_string(2))   # first two chars
print (car['wide_name'].read_wide_string(-1))  # until NUL

# Exercise dbgscript.read_wide_string.
#
print (dbgscript.read_wide_string(car['wide_name'].address))
print (dbgscript.read_wide_string(car['wide_name'].address, 1))
print (dbgscript.read_wide_string(car['wide_name'].address, 5))
print (dbgscript.read_wide_string(car['wide_name'].address, 500))
print (dbgscript.read_wide_string(car['wide_name'].address, -1))

# TODO: Negative tests.
