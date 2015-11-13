# Test utilities
#
def negative_test(expected_err)
  yield
rescue expected_err => e
  puts e.class
end

