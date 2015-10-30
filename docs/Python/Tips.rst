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

To leave the REPL, type ``quit()``.

Debugging with pdb
==================

`pdb`_ is the command line Python Debugger.

You can run it as a module via::

    0:031> !runscript -l py -- -m pdb E:\dev\dbgscripts_sql\python\hkbaseset.py
    > e:\dev\dbgscripts_sql\python\hkbaseset.py(3)<module>()
    -> """
    
Consult the `pdb`_ documentation for usage.

Sample session::

    0:031> !runscript -l py -- -m pdb E:\dev\dbgscripts_sql\python\hkbaseset.py
    > e:\dev\dbgscripts_sql\python\hkbaseset.py(3)<module>()
    -> """
    (Pdb) n
    n
    > e:\dev\dbgscripts_sql\python\hkbaseset.py(5)<module>()
    -> import sys, dbgscript
    (Pdb) n
    n
    > e:\dev\dbgscripts_sql\python\hkbaseset.py(7)<module>()
    -> def dump_base_set_bucket(bucket, link_offset, full_elem_type):
    (Pdb) b dump_base_set
    b dump_base_set
    Breakpoint 1 at e:\dev\dbgscripts_sql\python\hkbaseset.py:17
    (Pdb) b
    b
    Num Type         Disp Enb   Where
    1   breakpoint   keep yes   at e:\dev\dbgscripts_sql\python\hkbaseset.py:17
    (Pdb)
    
Debugging with Visual Studio
============================

You can debug Python in Visual Studio with the help of |ptvs|__.

Step 1
------

Start the VM and open a debugger communication channel::

    !startvm
    !evalstring -l py "import ptvsd; ptvsd.enable_attach(secret='my_secret')"

Where ``my_secret`` can be any string that you will later specify in the Visual
Studio attach dialog. See the `ptvsd`_ documentation for details.

Step 2
------

Open your script in Visual Studio and set any breakpoints desired. Or, you can
just let it run until an unhandled exception is encountered.

Run your script with ``-m ptvsdrun`` prefixed::

    !runscript -l py -- -m ptvsdrun <your script> [your script args]

At this stage, the debugger will be in a ``*BUSY*`` state, waiting for you to
attach.

Step 3
------

In Visual Studio, open the `Attach To Process` dialog (default
shortcut: Ctrl-Alt-P) and set the transport to `Python remote (ptvsd)`.

Set the qualifier to my_secret@localhost and hit Enter. Your debugger process
should show up in the list. Click Attach and off you go. You can now debug as
you do in Visual Studio.

Step 4
------

Detach from Visual Studio and clean up the VM with::

    !stopvm

This will destroy the Python (and other providers') virtual machines and reset
their state.

.. warning::

   ``ptvs.enable_attach`` creates a background thread in the debugger process
   and opens a socket. Unfortunately, Python cleanup is not comprehensive and
   will not clean up all background threads that it started on behalf of
   scripts.
   
   This means TODO

   

.. |ptvs| replace:: Python Tools for Visual Studio
__ http://microsoft.github.io/PTVS
.. _ptvsd: https://github.com/Microsoft/PTVS/wiki/Cross-Platform-Remote-Debugging#preparing-the-script-for-debugging
.. _REPL: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop
.. _pdb: https://docs.python.org/3.6/library/pdb.html