iface = DbgScript.create_typed_object('hkengtest!HkHostLog', 0x000005061ef0108)

puts iface.type, iface.address

# Downcast it.
concrete = iface.get_runtime_obj()

puts concrete.type, concrete.address, concrete.RefCount.value