DbgScript Reference
*******************

Debugger Entry Points
=====================

!runscript
----------

Synopsis
^^^^^^^^
.. code-block:: none

    !runscript [host-switches] [--] [provider-switches] <script-file> [arg1[, arg2[, ...]]]

Description
^^^^^^^^^^^

The only required parameter is the script file.

``host-switches`` are one or more of

  ``-l <lang-id>``
    Specifies the language the script is written in. This invokes
    the registered provider for this language id.
                
    See :ref:`3rd-party-config` for information on installing and
    configuring language providers.
    
  ``-t``
    Time the execution. Reports elapsed time at the end of script
    execution.
                
``--`` is the host-argument delimiter. It signals the host layer to stop
accepting further arguments for itself and pass the remainder to the provider
that was selected (either the default or via the ``-l`` switch). It is only
necessary to include it if the first argument to the provider is a switch
(i.e. starts with hypen (``-``)). If it is a filename, then this
delimiter is unnecessary.

``provider-switches`` are switches specific to the provider. Each provider
implements its own set of switches. Consult the documentation specific to the
provider to see what switches it offers.

Examples
^^^^^^^^

.. code-block:: none
    :linenos:
    
    !runscript E:\dev\dbgscript\samples\rb\stack_test.rb
    !runscript -l rb E:\dev\dbgscript\samples\rb\stack_test.rb
    !runscript -l rb -- E:\dev\dbgscript\samples\rb\stack_test.rb
    !runscript -l py E:\dev\dbgscript\samples\py\stack_test.py
    !runscript -l py -- E:\dev\dbgscript\samples\py\stack_test.py
    !runscript -l py -- -m pdb E:\dev\dbgscript\samples\py\stack_test.py
    !runscript -l py -t -- -m pdb E:\dev\dbgscript\samples\py\stack_test.py

You can use `!scriptpath`_ to shorten the path you need to pass to ``!runscript``.

!evalstring
-----------

Synopsis
^^^^^^^^
.. code-block:: none

    !evalstring [host-switches] [--] <script-string>
    
Description
^^^^^^^^^^^

``!evalstring`` can be used to evaluate an ad-hoc statement. ``host-switches``
are as in `!runscript`_. ``<script-string>`` is the string to be sent unmodified
to the selected script provider and executed.

Examples
^^^^^^^^

.. code-block:: none

    0:031> !evalstring -l rb puts DbgScript.get_threads
    #<DbgScript::Thread:0x000045f764b788>
    #<DbgScript::Thread:0x000045f764b760>
    ...

This example evaluates the statement ``puts DbgScript.get_threads`` in the Ruby
script provider. The output is shown in the debugger (in this case, an array
of Thread objects.)

.. code-block:: none

    0:000> !evalstring -l rb puts DbgScript.get_threads.map {|t| t.thread_id}
    14376
    20204
    4524
    18712
    ...

This example builds on the previous and dumps all the thread IDs.

!scriptpath
-----------

Synopsis
^^^^^^^^

.. code-block:: none
    :linenos:
    
    !scriptpath <path1>[,<path2>[,...]]
    !scriptpath
    
Description
^^^^^^^^^^^

Takes a comma-separated list of paths to search when running scripts via
`!runscript`_.

.. note:: 

    The separator is a `comma`, not semicolon. Semicolon is reserved
    by the debugger to separate commands.

Run with no arguments to see the current path list.

!startvm
--------

Synopsis
^^^^^^^^

.. code-block:: none

    !startvm
    
Description
^^^^^^^^^^^

By default, every script execution or string evaluation will recycle the script
provider's virtual machine. This means any functions or global variables you
define will be thrown away at the end of execution.

Sometimes you want to preserve the state of the execution -- perhaps to poke
around with ad-hoc statements, enter a `REPL`_, call arbitrary functions you've
previously defined, etc.

For this, you can call ``!startvm`` to instruct DbgScript to preserve the VM
state for all providers until `!stopvm`_ is called.

!stopvm
-------

Synopsis
^^^^^^^^

.. code-block:: none

    !stopvm
    
Description
^^^^^^^^^^^
Ends a persistent VM session started by `!startvm`_.


.. _REPL: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop
