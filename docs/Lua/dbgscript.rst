dbgscript Module
================

.. method:: currentThread() -> Thread

   Get the current thread in the process.

.. method:: getThreads() -> table of Thread

   Get the collection of threads in the process.
     
.. method:: createTypedObject(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt
   
.. method:: readPtr(addr) -> int

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
