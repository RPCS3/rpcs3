:: clean_msvc.cmd
::
:: This batch file cleans up some files that MSVC's Clean/Rebuild commands tend to miss.  In
:: particular the .ilk and .pdb files are known to get corrupted and cause all sorts of odd linker
:: linker errors, and the .ncb files can also get corrupted and cause intellisense breakges. 
::
:: Safety: This tool should be pretty safe.  It uses the command path to perform the deletion,
:: instead of relying on the CWD (which can sometimes fail to be set when working with UNCs across
:: network shares).  Furthermore, none of the files it deletes are important.  That is, they're
:: all files MSVC just rebuilds automatically next time you run/recompile.  The one minor
:: exception is *.pdb, since windows and a lot of developer tools include PDB sets to assist in
:: application debugging (however these files are by no means required by any software).

del /s "%~dp0\*.ncb"
del /s "%~dp0\*.obj"
del /s "%~dp0\bin\*.ilk"
del /s "%~dp0\*.idb"
del /s "%~dp0\*.bsc"
del /s "%~dp0\*.sbr"
del /s "%~dp0\*.pch"
del /s "%~dp0\*.pdb"

del /s /q "%~dp0\deps"

:: These two can't be used currently because they match unwanted 4+ letter extensions, such
:: as *.resx and *.tmpl ... wow, stupid. >_<

:: del /s "%~dp0\*.tmp"
:: del /s "%~dp0\*.res"
