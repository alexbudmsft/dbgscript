dbgscript Module
================
.. include:: ../shared/dbgscript_mod.txt

.. method:: dbgscript.getGlobal(symbol) -> TypedObject

   .. include:: ../shared/get_global.txt

.. method:: dbgscript.currentThread() -> Thread

   Get the current thread in the process.

.. method:: dbgscript.getThreads() -> table of Thread

   Get the collection of threads in the process.
     
.. method:: dbgscript.createTypedObject(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt
   
.. method:: dbgscript.createTypedPointer(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_pointer.txt
   
   .. versionadded:: 1.0.5
   
.. method:: dbgscript.readPtr(addr) -> integer

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
   
.. method:: dbgscript.readString(addr [, count]) -> string

   Read an ANSI string from the target process starting at `addr`.
   `count` (optional) specifies the maximum number of characters to read.

   .. versionadded:: 1.0.4
   
.. method:: dbgscript.readWideString(addr [, count]) -> string

   Read a wide string from the target process starting at `addr`.
   `count` (optional) specifies the maximum number of characters to read.

   .. versionadded:: 1.0.4
   
.. method:: dbgscript.readBytes(addr, count) -> string

   Read `count` bytes from `addr`.

   .. versionadded:: 1.0.3

.. method:: dbgscript.getNearestSym(addr) -> string

   Lookup the nearest symbol to address `addr`. Operates similar to the debugger
   ``ln`` command.

   .. versionadded:: 1.0.1
   
.. method:: dbgscript.getPeb() -> integer

   Get the address of the current process' PEB.

   .. versionadded:: 1.0.3
   
.. method:: dbgscript.fieldOffset(type, field) -> integer

   Obtain the offset of `field` in `type`. Behaves like ``offsetof`` macro in C.
   
   .. versionadded:: 1.0.2

.. method:: dbgscript.getTypeSize(type) -> integer

   Obtain the size of `type` in bytes. Behaves like ``sizeof`` operator in C.
   
   .. versionadded:: 1.0.4
   
.. method:: dbgscript.startBuffering()

   .. include:: ../shared/start_buffering.txt

.. method:: dbgscript.stopBuffering()

   .. include:: ../shared/stop_buffering.txt

.. method:: dbgscript.execCommand(cmd)

   Executes a debugger command `cmd` and prints the output.

.. method:: dbgscript.resolveEnum(enum, val) -> string

   Obtains the textual name of the enumerant given an enum `enum` and a value
   `val`.

