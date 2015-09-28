|logo| DbgScript |release|
**************************

DbgScript is a multi-script debugger extension.

More precisely, it is a `DbgEng extension`_ that allows the embedding of
arbitrary scripting languages into the `Debugging Tools for Windows`_.

DbgScript lets you harness the power of Ruby, Python or Lua to dump complex data
structures, mine data from dumps, conduct automated analyses, and more.

It also offers an extensibility model allowing new languages to be added with no
changes to the core.

DbgScript extends scripting languages with a API that models
elements in the target process like threads, C/C++ objects, stack frames, etc.

For example, in the Python Script Provider, a thread is modelled as a
Thread object with various attributes and methods that can obtain further
information about it, such as its call stack.

DbgScript comes with three first-party providers: `Python`_, `Ruby`_ and `Lua`_.

.. toctree::
   :hidden:
   
   Installation
   Tutorial
   Reference
   Python
   Ruby
   Lua

.. |logo| image:: logo.svg
    :width: 80 px
    :height: 80px

.. _`DbgEng extension`: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551059(v=vs.85).aspx
.. _`Debugging Tools for Windows`: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551063(v=vs.85).aspx
.. _`Python`: https://www.python.org
.. _`Ruby`: https://www.ruby-lang.org/en
.. _`Lua`: http://www.lua.org
