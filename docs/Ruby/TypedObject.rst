.. default-domain:: rb

.. currentmodule:: DbgScript

.. include:: ../shared/TypedObject_preamble.txt

.. class:: TypedObject

.. method:: TypedObject#name -> String

   Get the name of the object.
     
.. method:: TypedObject#address -> Integer

   Get the virtual address of the object.
   
.. method:: TypedObject#type -> String

   Get the type name of the object.
   
.. method:: TypedObject#module -> String

   Get the module name of the typed object.
   
.. method:: TypedObject#data_size -> Integer

   Get the size of the object in bytes.
   
.. method:: TypedObject#value

   Returns the value of the object, if it's a primitive type. Raises 
   :class:`TypeError` otherwise.

.. method:: obj[key]

   If `key` is an integer, attempts an array element access. If the object
   is not an array or pointer, raises :class:`RuntimeError`.

   If `key` is a string, attempts to look up a field with that name. If no such
   field exists, raises :class:`RuntimeError`.

.. method:: obj.field

   Secondary way of looking up fields. If `field` names a built-in attribute,
   that attribute is returned. Otherwise a field lookup is performed.

   This allows a more convenient way of traversing C/C++ objects. E.g.::

     foo.m_bar
     foo['m_bar']

   are equivalent. The explicit form is more performant though and recommended
   for tight loops.

.. method:: TypedObject#read_string([count]) -> String

   Read an ANSI string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject#read_wide_string([count]) -> String

   Read a wide string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject#read_bytes(count) -> String

   Read a block of `count` bytes from the target process from this object's
   address.

   .. versionadded:: 1.0.3

.. method:: TypedObject#deref -> TypedObject

   Dereference the current object, if it's a pointer or array.
   
.. method:: TypedObject#get_runtime_obj -> TypedObject

   Attempts to dynamically down-cast the current object if it has a vtable.

.. method:: 
	TypedObject#length -> Integer
	TypedObject#size -> Integer

   If this object represents an array, returns its length, otherwise raises a
   :class:`TypeError`.
