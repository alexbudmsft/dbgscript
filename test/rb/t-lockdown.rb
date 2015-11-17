require_relative 'utils'

negative_test(LoadError) { require 'socket.so' }
negative_test(NameError) { open }
negative_test(NameError) { File }
negative_test(NameError) { FileTest }
negative_test(NameError) { Process }
negative_test(NameError) { Fiber }
negative_test(NameError) { Thread }
negative_test(NameError) { Dir }
negative_test(NameError) { Signal }
