.. default-domain:: py

.. currentmodule:: dbgscript

TypedObject class
=================

.. class:: TypedObject

TypedObject represents a typed object in the target process's virtual address
space. It can be constructed from a type and an address, or by looking up fields
of a struct, dereferencing pointers, accessing array elements, and so on.

TypedObject attributes
----------------------

.. attribute:: TypedObject.name

   Get the name of the object, if available. Some objects have no name, for
   example if they were created from array elements. Returns a :class:`str`.
     
.. attribute:: TypedObject.address

   Get the virtual address of the object as an :class:`int`.
   
.. attribute:: TypedObject.type

   Get the type name of the object as a :class:`str`.
   
.. attribute:: TypedObject.size

   Get the size of the object in bytes as an :class:`int`.

.. TODO: Describe subscript operator (both int and str keys)