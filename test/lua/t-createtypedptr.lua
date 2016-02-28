require 'utils'

local car = getCar()

local addr = car:f('name').address

-- Obtain a pointer to the name buffer.
--
local tp = dbgscript.createTypedPointer('kernelbase!char', addr)

-- Print the first few characters.
--
print (tp[0].value)
print (tp[1].value)
