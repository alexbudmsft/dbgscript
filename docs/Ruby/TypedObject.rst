.. default-domain:: rb

.. currentmodule:: DbgScript

.. include:: ../shared/TypedObject_preamble.txt

.. class:: TypedObject

.. attr_reader:: TypedObject.name() -> String

   Get the name of the object.
     
.. attr_reader:: TypedObject.address() -> Integer

   Get the virtual address of the object.
   
.. attr_reader:: TypedObject.type() -> String

   Get the type name of the object.
   
.. attr_reader:: TypedObject.module() -> String

   Get the module name of the typed object.
   
.. attr_reader:: TypedObject.size() -> Integer

   Get the size of the object in bytes.

.. TODO: Describe subscript operator (both int and str keys)
