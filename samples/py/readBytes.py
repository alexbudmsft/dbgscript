import sys

addr = int(sys.argv[1], 0)
count = 10
car = dbgscript.create_typed_object('dummy1!Car', addr)
x = car.read_bytes(count)
print (x)

x = dbgscript.read_bytes(addr, count)
print (x)