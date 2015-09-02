.. default-domain:: py

.. currentmodule:: dbgscript

.. include:: ../shared/StackFrame_preamble.txt

.. class:: StackFrame

.. attribute:: StackFrame.frame_number() -> int

   Get the frame number of this stack frame.
   
.. attribute:: StackFrame.instruction_offset() -> int

   Get the virtual address of the frame's current instruction.

.. method:: StackFrame.get_locals() -> tuple of TypedObject

   Get the local variables in the stack frame.
   
.. method:: StackFrame.get_args() -> tuple of TypedObject

   Similar to :meth:`.get_locals` except return the arguments only.
   
