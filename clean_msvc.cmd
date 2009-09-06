:: clean_msvc.cmd
::
:: This batch file cleans up some files that MSVC's Clean/Rebuild commands tend to miss.
:: In particular the .ilk and .pdb files are known to get corrupted and cause all sorts of odd
:: linker errors, and the .ncb files can also get corrupted and cause intellisense breakges.
::
:: Safety: This tool should be pretty safe.  None of the files it deletes are important.  That
:: is, they're all files MSVC just rebuilds automatically next time you run/recompile.  But even
:: so, don't go running this batch file in your root c:\ folder.  It's probably not a wise action.
:: Enjoy. :)

del /s *.ncb;*.ilk;*.pdb;*.bsc;*.sbr;*.res