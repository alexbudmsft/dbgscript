.. default-domain:: py

.. module:: dbgscript

dbgscript Module
================

.. include:: ../shared/dbgscript_mod.txt

You do not need to ``import dbgscript``. It is already visible in the global
namespace.

.. method:: get_global(symbol) -> TypedObject

   .. include:: ../shared/get_global.txt

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
   
.. method:: read_bytes(addr, count) -> bytes

   Read `count` bytes from `addr`.
   
.. method:: get_nearest_sym(addr) -> str

   Lookup the nearest symbol to address `addr`. Operates similar to the debugger
   ``ln`` command.

.. method:: get_peb() -> int

   Get the address of the current process' PEB.

.. method:: field_offset(type, field) -> int

   Obtain the offset of `field` in `type`. Behaves like ``offsetof`` macro in C.
   
.. method:: start_buffering()

   .. include:: ../shared/start_buffering.txt

.. method:: stop_buffering()

   .. include:: ../shared/stop_buffering.txt

.. method:: execute_command(cmd)

   Executes a debugger command `cmd` and prints the output.

.. method:: resolve_enum(enum, val) -> str

   Obtains the textual name of the enumerant given an enum `enum` and a value
   `val`.
