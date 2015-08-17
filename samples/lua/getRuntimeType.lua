iface = dbgscript.createTypedObject('hkengtest!HkHostLog', 0x0000005061ef0108)

print(iface.type, iface.address)

-- Downcast it.
concrete = iface:getRuntimeObject()

print(concrete.type, concrete.address, concrete.RefCount.value)