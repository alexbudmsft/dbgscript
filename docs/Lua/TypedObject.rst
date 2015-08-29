TypedObject class
=================

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

Sequence support
----------------

If the TypedObject represents an array or pointer, the object implements the
sequence protocol.

If the object represents an array, its length can be determined with the
builtin :func:`len` function.

If the object represents an array or pointer, its elements can be obtained via
the subscript operator with an :class:`int` key::

    obj[0]  # obtains first element
    obj[5]  # obtains sixth element

Mapping support
---------------

.. TODO