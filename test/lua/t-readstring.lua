require 'utils'

local car = getCar()

-- Read ANSI string.
--
print(car:f('name'):readString())      -- until NUL
print(car:f('name'):readString(1))     -- first char
print(car:f('name'):readString(2))     -- first two chars
print(car:f('name'):readString(-1))    -- until NUL

-- Longer than the real length. Expect full string to be read without any
-- embedded NULs.
--
print(car:f('name'):readString(1000))

-- Exercise dbgscript.readString.
--
print(dbgscript.readString(car:f('name').address))
print(dbgscript.readString(car:f('name').address, 1))
print(dbgscript.readString(car:f('name').address, 5))
print(dbgscript.readString(car:f('name').address, 500))
print(dbgscript.readString(car:f('name').address, -1))

-- Read Wide string.
--
print(car:f('wide_name'):readWideString())      -- until NUL
print(car:f('wide_name'):readWideString(1))    -- first char
print(car:f('wide_name'):readWideString(2))    -- first two chars
print(car:f('wide_name'):readWideString(-1))   -- until NUL

-- Longer than the real length. Expect full string to be read without any embedded
-- NULs.
--
print(car:f('wide_name'):readWideString(1000))

-- Exercise dbgscript.readWideWideString.
--
print(dbgscript.readWideString(car:f('wide_name').address))
print(dbgscript.readWideString(car:f('wide_name').address, 1))
print(dbgscript.readWideString(car:f('wide_name').address, 5))
print(dbgscript.readWideString(car:f('wide_name').address, 500))
print(dbgscript.readWideString(car:f('wide_name').address, -1))
