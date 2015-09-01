.. include:: ../shared/Thread_preamble.txt

.. Nifty trick: Use '.. attribute' and append empty parens to the name to get
   the -> arrow to show up as an elongated arrow just as in method definitions.
   The parens won't actually show up, so it remains to look like an attribute.

.. attribute:: engineId() -> integer

   Get the dbgeng-local thread ID. This is not the Windows thread ID.
    
.. attribute:: threadId() -> integer

   Get the Windows thread ID.
   
.. attribute:: teb() -> integer

   Get the address of the TEB.
   
.. method:: currentFrame() -> StackFrame

   Get the current ``StackFrame``.

.. method:: getStack() -> table of StackFrame

   Get the current call stack.
