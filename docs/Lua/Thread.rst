Thread class
============

.. Nifty trick: Use '.. attribute' and append empty parens to the name to get
   the -> arrow to show up as an elongated arrow just as in method definitions.
   The parens won't actually show up, so it remains to look like an attribute.

.. attribute:: engineId() -> integer

   Get the dbgeng local thread id. This is not the Windows thread id.
    
.. attribute:: threadId() -> integer

   Get the Windows thread id.
   
.. attribute:: teb() -> integer

   Get the address of the TEB.
   
.. method:: currentFrame() -> StackFrame

   Get the current :class:`StackFrame`.

.. method:: get_stack() -> tuple of StackFrame

   Get the collection of threads in the process.
   
   For example::
   
       thd = ... # some thread
       stack = thd.get_stack()
       for frame in stack:
           print(frame.frame_number, frame.instruction_offset)
