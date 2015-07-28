from dbgscript import *
thd = Process.current_thread
print(thd)
frame = thd.current_frame
locals = frame.get_locals()
print(locals)
for l in locals: print(l.name)
for l in locals: print(l.name, l.type)
car1 = locals[0]
print(car1.name)
car1_f = car1['f']
print(car1_f)
print(car1_f.name, car1_f.type)
print(car1_f.name, car1_f.type, car1_f.size)
foo_c = car1_f['c']
print(foo_c)
print(foo_c.name)
print(foo_c.name, foo_c.type)
print(foo_c.name, foo_c.type, foo_c.size, hex(foo_c.address), foo_c.value)
# car1_f['xyz'] # no such field
print(car1_f['arr'])
print(car1_f['arr'][0].value)
print(len(car1_f['arr']))
some_foo_ptr = Process.read_ptr(0x000007f913ef9c0)
print(hex(some_foo_ptr))