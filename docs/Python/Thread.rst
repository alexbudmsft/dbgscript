.. default-domain:: py
.. currentmodule:: dbgscript

.. include:: ../shared/Thread_preamble.txt

.. class:: Thread

.. attribute:: Thread.engine_id

   Get the dbgeng local thread id. This is not the Windows thread id.

.. attribute:: Thread.thread_id

   Get the Windows thread id.
   
.. attribute:: Thread.teb

   Get the address of the TEB.
   
.. attribute:: Thread.current_frame

   Get the current :class:`StackFrame`.

.. method:: Thread.get_stack() -> tuple of StackFrame

   Get the current call stack.
   
   For example::
   
       thd = ... # some thread
       stack = thd.get_stack()
       for frame in stack:
           print(frame.frame_number, frame.instruction_offset)
