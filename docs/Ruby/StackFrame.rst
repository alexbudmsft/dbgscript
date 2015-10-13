.. default-domain:: rb

.. currentmodule:: DbgScript

.. include:: ../shared/StackFrame_preamble.txt

.. class:: StackFrame

.. attr_reader:: StackFrame#frame_number -> Integer

   Get the frame number of this stack frame.
   
.. attr_reader:: StackFrame#instruction_offset -> Integer

   Get the virtual address of the frame's current instruction.

.. method:: StackFrame#get_locals -> array of TypedObject

   Get the local variables in the stack frame.
   
.. method:: StackFrame#get_args -> array of TypedObject

   Similar to :meth:`#get_locals` except return the arguments only.
