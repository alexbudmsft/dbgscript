Basics
******

This section explains some basic usage of DbgScript. For details see the :doc:`Reference`.

Running Scripts
===============

To run a script, use the ``!runscript`` entry point. For example::

    !runscript -l rb E:\dev\dbgscript\samples\rb\stack_test.rb

You can use `!scriptpath`_ to shorten the path you need to pass to ``!runscript``.

                
Ad-hoc Evaluation
=================

``!evalstring`` can be used to evaluate an ad-hoc statement. For example::

    0:031> !evalstring -l rb puts DbgScript.get_threads
    #<DbgScript::Thread:0x000045f764b788>
    #<DbgScript::Thread:0x000045f764b760>
    ...

This example evaluates the statement ``puts DbgScript.get_threads`` in the Ruby
script provider. The output is shown in the debugger (in this case, an array
of Thread objects.)

Other Entry Points
==================

!scriptpath
-----------

This takes a comma-separated list of paths to search when running scripts. For example::

    !scriptpath e:\dev\rb_scripts,d:\pythonscripts
    
Note that the separator is a `comma`, not semicolon. Semicolon is reserved
by the debugger to separate commands.

Run with no arguments to see the current path list.