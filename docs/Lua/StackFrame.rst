.. include:: ../shared/StackFrame_preamble.txt

.. attribute:: frameNumber() -> integer

   Get the frame number of this stack frame.
   
.. attribute:: instructionOffset() -> integer

   Get the virtual address of the frame's current instruction.
   
.. method:: getLocals() -> table of TypedObject

   Get the local variables in the stack frame. This includes arguments.
   
.. method:: getArgs() -> table of TypedObject

   Similar to ``getLocals`` except returns the arguments only.
   