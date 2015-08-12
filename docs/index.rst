|logo| DbgScript
====================================================

DbgScript is a multi-script debugger extension.

More precisely, it is a `DbgEng extension`_ that allows the embedding of
arbitrary scripting languages into the `Debugging Tools for Windows`_.

It offers an extensibility model allowing new languages to be added with no
changes to the core.

DbgScript extends the scripting languages with a typed object model that models
elements in the target process like threads, (C++) objects, stack frames, etc.

For example, in the Python Script Provider, a thread is modelled as a
Thread object with various attributes and methods that can obtain further
information about it, such as its call stack.

DbgScript comes with two first-party providers: `Python`_ and `Ruby`_.

Contents:

.. toctree::
   :maxdepth: 2
   
   Basics
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

.. _`DbgEng extension`: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551059(v=vs.85).aspx
.. _`Debugging Tools for Windows`: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063(v=vs.85).aspx
.. _`Python`: https://www.python.org
.. _`Ruby`: https://www.ruby-lang.org/en