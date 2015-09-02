.. default-domain:: py

.. module:: dbgscript

dbgscript Module
================

.. include:: ../shared/dbgscript_mod.txt

You do not need to ``import dbgscript``. It is already visible in the global
namespace.

.. method:: current_thread() -> Thread

   Get the current thread in the process.

.. method:: get_threads() -> tuple of Thread

   Get the collection of threads in the process.
     
.. method:: create_typed_object(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt
   
.. method:: read_ptr(addr) -> int

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
