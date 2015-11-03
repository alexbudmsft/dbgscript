t = dbgscript.current_thread()
f = t.current_frame
locals = f.get_locals()

# Get the 'car' local. Throws StopIteration if not found.
#
car = next(t for t in locals if t.name == 'car')
print (car.x.value, car.y.value)

print("Wheel diameters:")
for i in range(len(car.wheels)):
	wheel = car.wheels[i]
	print("{:.2f}".format(wheel.diameter.value))