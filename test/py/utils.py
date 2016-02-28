import dbgscript

def get_car():
  t = dbgscript.current_thread()
  f = t.current_frame

  # 'locals' is a Python built-in function.
  #
  f_locals = f.get_locals()

  # Get the 'car' local. Throws StopIteration if not found.
  #
  car = next(t for t in f_locals if t.name == 'car')

  return car
