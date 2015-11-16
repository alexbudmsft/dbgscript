.. default-domain:: rb
.. currentmodule:: DbgScript

.. include:: ../shared/Thread_preamble.txt

.. class:: Thread

.. method:: Thread#engine_id -> Integer

   Get the dbgeng local thread id. This is not the Windows thread id.

.. method:: Thread#thread_id -> Integer

   Get the Windows thread id.
   
.. method:: Thread#teb -> Integer

   Get the address of the TEB.
   
.. method:: Thread#current_frame -> StackFrame

   Get the current :class:`StackFrame`.

.. method:: Thread#get_stack -> array of StackFrame

   Get the current call stack.
   
