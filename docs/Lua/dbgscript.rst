dbgscript Module
================
.. include:: ../shared/dbgscript_mod.txt

.. method:: dbgscript.currentThread() -> Thread

   Get the current thread in the process.

.. method:: dbgscript.getThreads() -> table of Thread

   Get the collection of threads in the process.
     
.. method:: dbgscript.createTypedObject(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt
   
.. method:: dbgscript.readPtr(addr) -> integer

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
