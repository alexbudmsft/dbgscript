.. default-domain:: py

.. currentmodule:: dbgscript

.. include:: ../shared/TypedObject_preamble.txt

.. class:: TypedObject

.. attribute:: TypedObject.name() -> str

   Get the name of the object.
     
.. attribute:: TypedObject.address() -> int

   Get the virtual address of the object.
   
.. attribute:: TypedObject.type() -> str

   Get the type name of the object.
   
.. attribute:: TypedObject.size() -> int

   Get the size of the object in bytes.

.. attribute:: TypedObject.value()

   Returns the value of the object, if it's a primitive type. Raises 
   :class:`ValueError` otherwise.
   
.. attribute:: TypedObject.module() -> str

   Get the module name of the typed object.

.. method:: obj[key]

   If `key` is an integer, attempts an array element access. If the object
   is not an array or pointer, raises :class:`RuntimeError`.

   If `key` is a string, attempts to look up a field with that name. If no such
   field exists, raises :class:`RuntimeError`.

.. attribute:: obj.field

   Secondary way of looking up fields. If `field` names a built-in attribute,
   that attribute is returned. Otherwise a field lookup is performed.

   This allows a more convenient way of traversing C/C++ objects. E.g.::

     foo.m_bar
     foo['m_bar']

   are equivalent. The explicit form is more performant though and recommended
   for tight loops.

.. method:: TypedObject.read_string([count]) -> str

   Read an ANSI string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject.read_wide_string([count]) -> str

   Read a wide string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject.read_bytes(count) -> bytes

   Read a block of `count` bytes from the target process from this object's
   address.
   
.. attribute:: TypedObject.deref() -> TypedObject

   Dereference the current object, if it's a pointer or array.
   
.. method:: TypedObject.get_runtime_obj() -> TypedObject

   Attempts to dynamically down-cast the current object if it has a vtable.

.. function:: builtin.len(obj) -> int

   If this object represents an array, returns its length, otherwise raises an
   :class:`ValueError`.

