|logo| DbgScript
====================================================

DbgScript is a multi-script debugger extension.

More precisely, it is a `DbgEng extension <https://msdn.microsoft.com/en-us/library/windows/hardware/ff551059(v=vs.85).aspx>`_ 
that allows the embedding of arbitrary scripting languages into the
`Debugging Tools for Windows <https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063(v=vs.85).aspx>`_.

It offers an extensibility model allowing new languages to be added with no
changes to the core.

DbgScript extends the scripting languages with a typed object model that models
elements in the target process like threads, (C++) objects, stack frames, etc.

For example, in the Python Script Provider, a thread is modelled as a
Thread object with various attributes and methods that can obtain further
information about it, such as its call stack.

DbgScript ships with two first-party providers: Python and Ruby.

Contents:

.. toctree::
   :maxdepth: 2
   
   Python
   Ruby


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

.. |logo| image:: logo.svg
    :width: 64px
    :height: 64px
