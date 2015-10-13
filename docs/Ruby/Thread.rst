.. default-domain:: rb
.. currentmodule:: DbgScript

.. include:: ../shared/Thread_preamble.txt

.. class:: Thread

.. attr_reader:: Thread#engine_id -> Integer

   Get the dbgeng local thread id. This is not the Windows thread id.

.. attr_reader:: Thread#thread_id -> Integer

   Get the Windows thread id.
   
.. attr_reader:: Thread#teb -> Integer

   Get the address of the TEB.
   
.. attr_reader:: Thread#current_frame -> StackFrame

   Get the current :class:`StackFrame`.

.. method:: Thread#get_stack -> array of StackFrame

   Get the current call stack.
   
