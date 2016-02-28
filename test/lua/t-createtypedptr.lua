require 'utils'

local car = getCar()

local addr = car:f('name').address

-- Obtain a pointer to the name buffer.
--
local tp = dbgscript.createTypedPointer('kernelbase!char', addr)

-- Print the first few characters.
--
print (string.char(tp[0].value))  -- TODO: Unify .value in Python/Ruby/Lua
print (string.char(tp[1].value))
