.. default-domain:: rb

.. module:: DbgScript

DbgScript Module
================
.. include:: ../shared/dbgscript_mod.txt

.. method:: DbgScript.get_global(symbol) -> TypedObject

   .. include:: ../shared/get_global.txt

.. method:: DbgScript.current_thread

   Get the current thread in the process.

.. method:: DbgScript.create_typed_object(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_object.txt

.. method:: DbgScript.create_typed_pointer(type, addr) -> TypedObject

   .. include:: ../shared/create_typed_pointer.txt
   .. versionadded:: 1.0.5
 
.. method:: DbgScript.get_threads() -> array of Thread

   Get the collection of threads in the process.
   
.. method:: DbgScript.read_ptr(addr) -> Integer

   Read a pointer value from the virtual address space of the target process.
   ``addr`` must be a valid (accessible) address. This will be 8 bytes on an
   x64 target.
   
.. method:: DbgScript.read_string(addr [, count]) -> String

   Read an ANSI string from the target process starting at `addr`.
   `count` (optional) specifies the maximum number of characters to read.

   .. versionadded:: 1.0.4
   
.. method:: DbgScript.read_wide_string(addr [, count]) -> String

   Read a wide string from the target process starting at `addr`.
   `count` (optional) specifies the maximum number of characters to read.

   .. versionadded:: 1.0.4
   
.. method:: DbgScript.read_bytes(addr, count) -> String

   Read `count` bytes from `addr`.

   .. versionadded:: 1.0.3
   
.. method:: DbgScript.get_nearest_sym(addr) -> String

   Lookup the nearest symbol to address `addr`. Operates similar to the debugger
   ``ln`` command.
   
   .. versionadded:: 1.0.1

.. method:: DbgScript.get_peb() -> Integer

   Get the address of the current process' PEB.
   
   .. versionadded:: 1.0.3

.. method:: DbgScript.field_offset(type, field) -> Integer

   Obtain the offset of `field` in `type`. Behaves like ``offsetof`` macro in C.
   
   .. versionadded:: 1.0.2
   
.. method:: DbgScript.get_type_size(type) -> Integer

   Obtain the size of `type` in bytes. Behaves like ``sizeof`` operator in C.
   
   .. versionadded:: 1.0.4

.. method:: DbgScript.search_memory(start, size, pattern, pattern_granularity) -> Integer

   Search the address space from [`start`, `start` + `size`) for `pattern`.

   :param Integer start: Start of address space to search.
   :param Integer size: Amount of bytes to search.
   :param String pattern: Pattern to search for. `pattern.size` must be a multiple of
      `pattern_granularity`.
   :param Integer pattern_granularity: Only consider matches at this granularity.
   :return: location of match.
   :rtype: Integer
   :raises KeyError: if no match found.

   .. versionadded:: 1.0.6

.. method:: DbgScript.start_buffering()

   .. include:: ../shared/start_buffering.txt

.. method:: DbgScript.stop_buffering()

   .. include:: ../shared/stop_buffering.txt

.. method:: DbgScript.execute_command(cmd)

   Executes a debugger command `cmd` and prints the output.

.. method:: DbgScript.resolve_enum(enum, val) -> String

   Obtains the textual name of the enumerant given an enum `enum` and a value
   `val`.

