# DbgScript Lockdown notes

This is a special flavor of DbgScript and its script providers designed for use
in environments not on the developer's machine, such as remote debugging
sessions where the debugging server would not like DbgScripts to read/write
arbitrary files, create arbitrary processes, open network sockets, etc.

This flavor should only be used in those situations: if running DbgScript on
your own machine the regular flavor is recommended.

A new symbol is introduced into the build system, `LOCKDOWN`, which allows for the
compile-time conditional exclusion of certain functionality.

The locked-down versions of the binaries have a `-lockdown` suffix for visibility.

The following sections list the specific lockdowns.

## Overall Lockdown

Currently, only the Ruby script provider ships with the locked down distribution.
This is because the other providers have not yet been locked down. Once they are,
they will be available too.

## Provider-specific lockdown

### Ruby

Several core classes have been undefined:

 * File
 * FileTest
 * Process
 * Fiber
 * Thread
 * Dir
 * Signal

In addition, all of the shipping extension modules have been blocked:

 * bigdecimal.so
 * continuation.so
 * coverage.so
 * date_core.so
 * digest.so
 * digest/bubblebabble.so
 * digest/md5.so
 * digest/rmd160.so
 * digest/sha1.so
 * digest/sha2.so
 * etc.so
 * fcntl.so
 * fiber.so
 * io/console.so
 * io/nonblock.so
 * json/ext/generator.so
 * json/ext/parser.so
 * mathn/complex.so
 * mathn/rational.so
 * nkf.so
 * objspace.so
 * pathname.so
 * psych.so
 * racc/cparse.so
 * rbconfig/sizeof.so
 * ripper.so
 * sdbm.so
 * socket.so
 * stringio.so
 * strscan.so
 * thread.so
 * win32ole.so
 
This means standard library scripts depending on them will cease to work.
