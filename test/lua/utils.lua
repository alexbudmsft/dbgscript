-- adapted from http://stackoverflow.com/a/664557/112950
--
function table.find(l, f) -- find element v of l satisfying f(v)
  for _, v in ipairs(l) do
    if f(v) then
      return v
    end
  end
  return nil
end

function getCar()
  local t = dbgscript.currentThread()
  local f = t:currentFrame()
  local locals = f:getLocals()

  -- Get the 'car' local.
  --
  return table.find(locals, function (e) return e.name == 'car' end)
end
