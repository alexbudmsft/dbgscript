Tips and Tricks
***************

Dropping into a REPL
====================

You can drop into an interactive `REPL`_ with::
    
    !evalstring -l py "import code; code.interact(local=locals())"

This is essentially the same as if you were to run the Python interactive
interpreter::

    0:031> !evalstring -l py "import code; code.interact(local=locals())"
    Python 3.6.0a0 (default:262c43c71927+, Jul 23 2015, 13:46:09) [MSC v.1900 64 bit (AMD64)] on win32
    Type "help", "copyright", "credits" or "license" for more information.
    (InteractiveConsole)
    >>> import dbgscript
    import dbgscript
    >>> dbgscript.get_threads
    dbgscript.get_threads
    <built-in function get_threads>
    >>> dbgscript.get_threads()
    dbgscript.get_threads()
    (<dbgscript.Thread object at 0x00000045F0A0BE50>, ...<snip>, <dbgscript.Thread object at 0x00000045F0A0BEE0>)
    >>> 

To leave the REPL, just hit Enter (i.e. pass an empty string.)

.. _REPL: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop