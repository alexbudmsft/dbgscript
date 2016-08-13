require 'utils'

local car = getCar()

-- Positive cases. Can't print the result because the address may change
-- from run to run.
--
dbgscript.searchMemory(car:f('name').address-16, 100, 'FooCar', 1)
dbgscript.searchMemory(car:f('name').address-16, 100, 'FooCar', 2)

-- Negative cases.
--

-- 4 is not a multiple of the pattern length.
-- Can't print 'err' because it contains full path of script.
--
local status, err = pcall(function()
  dbgscript.searchMemory(car:f('name').address-16, 100, 'FooCar', 4)
end)
print(status)

-- Try a non-existent pattern.
--
status, err = pcall(function()
  dbgscript.searchMemory(car:f('name').address-16, 100, 'AbcDefAb', 4)
end)
print(status)

-- 3 is a multiple of the pat. len, but the pattern won't be found on a
-- 3 byte granularity.
--
status, err = pcall(function()
  dbgscript.searchMemory(car:f('name').address-16, 100, 'FooCar', 3)
end)
print(status)
