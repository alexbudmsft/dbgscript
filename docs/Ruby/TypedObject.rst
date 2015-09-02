.. default-domain:: rb

.. currentmodule:: DbgScript

.. include:: ../shared/TypedObject_preamble.txt

.. class:: TypedObject

.. attr_reader:: TypedObject#name() -> String

   Get the name of the object.
     
.. attr_reader:: TypedObject#address() -> Integer

   Get the virtual address of the object.
   
.. attr_reader:: TypedObject#type() -> String

   Get the type name of the object.
   
.. attr_reader:: TypedObject#module() -> String

   Get the module name of the typed object.
   
.. attr_reader:: TypedObject#data_size() -> Integer

   Get the size of the object in bytes.
   
.. attr_reader:: TypedObject#value()

   Returns the value of the object, if it's a primitive type. Raises 
   :class:`ArgError` otherwise.

.. method:: obj[key]

   If `key` is an integer, attempts an array element access. If the object
   is not an array or pointer, raises :class:`RuntimeError`.

   If `key` is a string, attempts to look up a field with that name. If no such
   field exists, raises :class:`RuntimeError`.

.. attr_reader:: obj.field

   Secondary way of looking up fields. If `field` names a built-in attribute,
   that attribute is returned. Otherwise a field lookup is performed.

   This allows a more convenient way of traversing C/C++ objects. E.g.::

     foo.m_bar
     foo['m_bar']

   are equivalent. The explicit form is more performant though and recommended
   for tight loops.

.. method:: TypedObject#read_string([count])

   Read an ANSI string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject#read_wide_string([count])

   Read a wide string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject#deref()

   Dereference the current object, if it's a pointer or array.
   
.. method:: TypedObject#get_runtime_obj()

   Attempts to dynamically down-cast the current object if it has a vtable.

.. method:: 
	TypedObject#length()
	TypedObject#size()

   If this object represents an array, returns its length, otherwise raises an
   :class:`ArgError`.
