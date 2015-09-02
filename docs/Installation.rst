Installation
************

Run ``install.bat``. Assuming you have the `Prerequisites`_, you're good to go!

Run ``.load C:\Program Files\DbgScript\dbgscript.dll`` to load DbgScript in
your debugger of choice. 

.. note::

	Default path shown above. DbgScript installs into ``%ProgramFiles%``.

Prerequisites
=============

The Visual Studio `2015`_ and `2013`_ x64 CRT Redists are required.

.. note::

	You do not need to install Python or Ruby as their core files are bundled with
	the provider.

.. _3rd-party-config:

3rd Party Provider Registration
===============================
The ``install.bat`` script will automatically install all the first-party providers.

If you wish to install your own, add them to the
``HKCU\Software\Microsoft\DbgScript\Providers`` registry key.

The values in this key are of type REG_SZ and contain the `language identifier`
of the provider. The data associated with the key is the full path to the DLL
the provider is implemented in. For example::

    HKEY_CURRENT_USER\Software\Microsoft\DbgScript\Providers
        py    REG_SZ    E:\dev\dbgscript\deploy\debug\pythonprov\pythonprov.dll
        rb    REG_SZ    E:\dev\dbgscript\deploy\debug\rubyprov\rubyprov.dll
        
.. _`2013`: https://www.microsoft.com/en-us/download/details.aspx?id=40784
.. _`2015`: https://www.microsoft.com/en-us/download/details.aspx?id=48145