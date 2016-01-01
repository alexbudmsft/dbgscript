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
