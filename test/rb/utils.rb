# Test utilities
#
def negative_test(expected_err)
  yield
rescue expected_err => e
  puts e.class
end

def get_car
  t = DbgScript.current_thread
  f = t.current_frame
  locals = f.get_locals

  # Get the 'car' local.
  #
  locals.find {|t| t.name == 'car'}
end
