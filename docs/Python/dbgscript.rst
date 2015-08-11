.. default-domain:: py

.. module:: dbgscript

dbgscript Module
================

.. method:: current_thread() -> Thread

   Get the current thread in the process.

.. method:: get_threads() -> tuple(Thread)

   Get the collection of threads in the process.
     
.. method:: create_typed_object(type, addr) -> TypedObject

   Create a :class:`TypedObject` from an address and type.
   
.. note::

   Prefer using the fully-qualfied type name (i.e. including the module prefix)
   as it will accelerate symbol lookup dramatically. I.e. ``module!Foo``, not
   ``Foo``.
   
.. method:: read_ptr(addr) -> int

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
