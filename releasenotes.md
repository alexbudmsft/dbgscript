1.0.6 (beta)
------------

* Add `dbgscript.search_memory` API. Searches memory for a pattern and returns
  location on success.

1.0.5 (beta)
------------

* Add `dbgscript.create_typed_pointer` API. Acts like `create_typed_object` but
  returns a pointer to the object instead. This makes buffer traversal possible
  from an explicit buffer start address and element type.

* Fix: `TypedObject.value` API now acts consistently across providers for the
  `char` type. Previously, Python would return a `str` but Ruby/Lua would return
  an integer type.

1.0.4 (beta)
------------

* Add `dbgscript.get_type_size(type)` API.
* Add `dbgscript.read_[wide_]string` APIs. These were previously only available
  on TypedObject, but now can be invoked at the top-level without an object.

1.0.3 (beta)
------------

* Add `dbgscript.read_bytes(addr, count)` API.
* Add `TypedObject.read_bytes(count)` API.
* Add `dbgscript.get_peb()` API. Fetches address of PEB.

1.0.2 (beta)
------------

* Add `field_offset` API. Acts like `offsetof` macro in C.

1.0.1 (beta)
------------

* Add `get_nearest_sym` API. Mimics debugger `ln` command.

1.0.0 - Initial Release
-----------------------
