.. include:: ../shared/TypedObject_preamble.txt

.. attribute:: TypedObject.name() -> string

   Get the name of the object.
     
.. attribute:: TypedObject.address() -> integer

   Get the virtual address of the object as an :class:`int`.
   
.. attribute:: TypedObject.type() -> string

   Get the type name of the object.

.. attribute:: TypedObject.module() -> string

   Get the module name of the typed object.

.. attribute:: TypedObject.size() -> integer

   Get the size of the object in bytes.

.. attribute:: TypedObject.value()

   Returns the value of the object, if it's a primitive type. Raises 
   an error otherwise.
   
.. attribute::
	obj[key]
	obj.key
	
   If `key` is an integer, attempts an array element access. If the object
   is not an array or pointer, raises an error.

   If `key` is a string, and does **not** name a built-in property, attempts to
   look up a field with that name. If no such field exists, raises an error.

   The following are equivalent::

     foo.m_bar
     foo['m_bar']
     foo.f('m_bar')
     foo.field('m_bar')
     
.. method::
	obj.f(f)
	obj.field(f)
	
   Explicit way of looking up a field. `f` names the field to look up. If the 
   field doesn't exist, raises an error.

   This method is necessary if the field sought is hidden by a built-in 
   property, like ``name``. In that case, ``obj.name`` and ``obj['name']`` will
   yield the ``name`` property instead of performing a field lookup.

.. method:: TypedObject.readString([count]) -> string

   Read an ANSI string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.
   
.. method:: TypedObject.readWideString([count]) -> string

   Read a wide string from the target process starting at this object's 
   address. `count` (optional) specifies the maximum number of characters to read.

.. method:: TypedObject.readBytes(count) -> string

   Read a block of `count` bytes from the target process from this object's
   address.
   
.. attribute:: TypedObject.deref() -> TypedObject

   Dereference the current object, if it's a pointer or array.
   
.. method:: TypedObject.getRuntimeObject() -> TypedObject

   Attempts to dynamically down-cast the current object if it has a vtable.

.. method:: #obj

   If this object represents an array, returns its length, otherwise raises an
   error.

