.. py:currentmodule:: dbgscript
.. rb:currentmodule:: DbgScript

Thread
=================

Python 

.. py:class:: Thread

Ruby

.. rb:class:: Thread

Attributes
----------------------

Python

.. py:attribute:: Thread.engine_id

Ruby

.. rb:attr_reader:: Thread.engine_id

   The debugger engine thread identifier.
   
   :rtype: int
   
.. py:attribute:: Thread.thread_id

   Get the name of the object, if available. Some objects have no name, for
   example if they were created from array elements. Returns a :class:`Thread`.
   
.. py:attribute:: Thread.teb

   Get the name of the object, if available. Some objects have no name, for
   example if they were created from array elements. Returns a :class:`Thread`.
   
.. py:attribute:: Thread.current_frame

   Get the name of the object, if available. Some objects have no name, for
   example if they were created from array elements. Returns a :class:`Thread`.
   
Thread methods
----------------------

.. method:: Thread.get_stack()

   Get the collection of threads in the process. Returns a tuple of
   :class:`StackFrame` objects.
