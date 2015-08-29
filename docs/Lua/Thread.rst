Thread class
============

.. attribute:: engineId -> int

   Get the dbgeng local thread id. This is not the Windows thread id.

.. attribute:: threadId -> int

   Get the Windows thread id.
   
.. attribute:: teb (TODO)

   Get the address of the TEB.
   
.. attribute:: currentFrame() -> StackFrame

   Get the current :class:`StackFrame`.

.. method:: get_stack() -> tuple(StackFrame)

   Get the collection of threads in the process.
   
   For example::
   
       thd = ... # some thread
       stack = thd.get_stack()
       for frame in stack:
           print(frame.frame_number, frame.instruction_offset)
