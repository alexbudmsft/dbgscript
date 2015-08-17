iface = dbgscript.create_typed_object('hkengtest!HkHostLog', 0x000005061ef0108)

print(iface.type, iface.address)

# Downcast it.
concrete = iface.get_runtime_obj()

print(concrete.type, concrete.address, concrete.RefCount.value)