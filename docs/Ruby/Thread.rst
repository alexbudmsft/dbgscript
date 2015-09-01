.. default-domain:: rb
.. currentmodule:: DbgScript

.. include:: ../shared/Thread_preamble.txt

.. class:: Thread

.. attr_reader:: engine_id

   Get the dbgeng local thread id. This is not the Windows thread id.

.. attr_reader:: thread_id

   Get the Windows thread id.
   
.. attr_reader:: teb

   Get the address of the TEB.
   
.. attr_reader:: current_frame

   Get the current :class:`StackFrame`.

.. method:: get_stack() -> tuple(StackFrame)

   Get the collection of threads in the process.
   
