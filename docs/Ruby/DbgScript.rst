.. default-domain:: rb

.. module:: DbgScript

DbgScript Module
================
.. include:: ../shared/dbgscript_mod.txt

.. method:: get_global(symbol) -> TypedObject

   .. include:: ../shared/get_global.txt

.. attr_reader:: current_thread

   Get the current thread in the process.

.. method:: create_typed_object(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt
   
.. method:: get_threads() -> array of Thread

   Get the collection of threads in the process.
   
.. method:: read_ptr(addr) -> Integer

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
   
.. method:: read_bytes(addr, count) -> String

   Read `count` bytes from `addr`.
   
.. method:: get_nearest_sym(addr) -> String

   Lookup the nearest symbol to address `addr`. Operates similar to the debugger
   ``ln`` command.

.. method:: get_peb() -> Integer

   Get the address of the current process' PEB.

.. method:: field_offset(type, field) -> Integer

   Obtain the offset of `field` in `type`. Behaves like ``offsetof`` macro in C.
   
.. method:: start_buffering()

   .. include:: ../shared/start_buffering.txt

.. method:: stop_buffering()

   .. include:: ../shared/stop_buffering.txt

.. method:: execute_command(cmd)

   Executes a debugger command `cmd` and prints the output.

.. method:: resolve_enum(enum, val) -> String

   Obtains the textual name of the enumerant given an enum `enum` and a value
   `val`.

