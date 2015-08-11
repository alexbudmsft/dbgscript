.. default-domain:: py
.. currentmodule:: dbgscript
.. class:: Thread

Thread class
============

Attributes
----------------------

.. attribute:: engine_id

   Get the dbgeng local thread id. This is not the Windows thread id.

.. attribute:: thread_id

   Get the Windows thread id.
   
.. attribute:: teb

   Get the address of the TEB.
   
.. attribute:: current_frame

   Get the current :class:`StackFrame`.
   
Thread methods
----------------------

.. method:: get_stack() -> tuple(StackFrame)

   Get the collection of threads in the process.
   
   For example::
   
       thd = ... # some thread
       stack = thd.get_stack()
       for frame in stack:
           print(frame.frame_number, frame.instruction_offset)
