.. default-domain:: py

.. currentmodule:: dbgscript

Process class
=================

.. class:: Process


Process attributes
----------------------

.. attribute:: Process.current_thread

   Get the name of the object, if available. Some objects have no name, for
   example if they were created from array elements. Returns a :class:`Thread`.
     
Process methods
----------------------

.. method:: Process.get_threads()

   Get the collection of threads in the process. Returns a tuple of
   :class:`Thread` objects.
     
.. method:: Process.create_typed_object(type, addr)

   Create a :class:`TypedObject` from an address and type.
   
.. method:: Process.read_ptr(addr)

   Read a pointer from the virtual address space of the target process.
   
   :param int addr: The virtual address to read from. This must be a valid
                    (accessible) address.
   :rtype: int 
