puts DbgScript.field_offset('nt!GUID', 'Data1') # -> 0
puts DbgScript.field_offset('nt!GUID', 'Data2') # -> 4
puts DbgScript.field_offset('nt!GUID', 'Data3') # -> 6
puts DbgScript.field_offset('nt!GUID', 'Data4') # -> 8