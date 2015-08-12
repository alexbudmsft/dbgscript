.. default-domain:: py

.. currentmodule:: dbgscript

StackFrame class
=================

.. class:: StackFrame

The StackFrame class represents a stack frame in the target process. From it,
the locals and arguments can be obtained.

StackFrame attributes
----------------------

.. attribute:: StackFrame.frame_number

   Get the frame number of this stack frame as :class:`int`.
   
.. attribute:: StackFrame.instruction_offset

   Get the virtual address of the frame's current instruction as :class:`int`.
   
StackFrame methods
----------------------

.. method:: StackFrame.get_locals()

   Get the local variables in the stack frame as a tuple of :class:`TypedObject`.
   
.. method:: StackFrame.get_args()

   Similar to :meth:`.get_locals` except return the arguments only.
   
Here's an example showing these methods in action:

.. literalinclude:: ../../samples/py/stack_test.py
