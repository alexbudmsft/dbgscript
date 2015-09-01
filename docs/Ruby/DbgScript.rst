.. default-domain:: rb

.. module:: DbgScript

DbgScript Module
================
.. include:: ../shared/dbgscript_mod.txt

.. attr_reader:: current_thread

   Get the current thread in the process.

.. method:: create_typed_object(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt

.. method:: read_ptr(addr) -> Integer

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
