require_relative 'utils'

# Test locked down core classes.
#
negative_test(NameError) { open }
negative_test(NameError) { File }
negative_test(NameError) { FileTest }
negative_test(NameError) { Process }
negative_test(NameError) { Fiber }
negative_test(NameError) { Thread }
negative_test(NameError) { Dir }
negative_test(NameError) { Signal }

# Test locked down extensions.
#
locked_down_extensions = %w{
	bigdecimal.so
	continuation.so
	coverage.so
	date_core.so
	digest.so
	digest/bubblebabble.so
	digest/md5.so
	digest/rmd160.so
	digest/sha1.so
	digest/sha2.so
	etc.so
	fcntl.so
	fiber.so
	io/console.so
	io/nonblock.so
	json/ext/generator.so
	json/ext/parser.so
	mathn/complex.so
	mathn/rational.so
	nkf.so
	objspace.so
	pathname.so
	psych.so
	racc/cparse.so
	rbconfig/sizeof.so
	ripper.so
	sdbm.so
	socket.so
	stringio.so
	strscan.so
	thread.so
	win32ole.so
}

locked_down_extensions.each do |e|
  negative_test(LoadError) { require e }
end
