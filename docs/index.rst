DbgScript documentation
=======================

DbgScript is a `DbgEng extension <https://msdn.microsoft.com/en-us/library/windows/hardware/ff551059(v=vs.85).aspx>`_ 
that allows the embedding of arbitrary scripting languages into the
`Debugging Tools for Windows <https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063(v=vs.85).aspx>`_.

It is extensible so that new languages may be added in the future with no changes
to DbgScript itself.

DbgScript extends the scripting languages with a typed object model that models the
target process. For example, in the Python API, a thread is modelled as a
Thread object with various attributes and methods that can obtain further
information about it, such as its call stack.

There are two languages currently available: Python and Ruby.

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

