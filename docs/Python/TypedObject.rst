.. default-domain:: py

.. currentmodule:: dbgscript

.. include:: ../shared/TypedObject_preamble.txt

.. class:: TypedObject

.. attribute:: TypedObject.name() -> str

   Get the name of the object.
     
.. attribute:: TypedObject.address() -> int

   Get the virtual address of the object as an :class:`int`.
   
.. attribute:: TypedObject.type() -> str

   Get the type name of the object as a :class:`str`.
   
.. attribute:: TypedObject.size() -> int

   Get the size of the object in bytes as an :class:`int`.
   
.. attribute:: TypedObject.module() -> str

   Get the module name of the typed object.
   
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