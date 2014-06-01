simpleini
=========

A cross-platform library that provides a simple API to read and write INI-style configuration files. It supports data files in ASCII, MBCS and Unicode. It is designed explicitly to be portable to any platform and has been tested on Windows, WinCE and Linux. Released as open-source and free using the MIT licence.

# Feature Summary

- MIT Licence allows free use in all software (including GPL and commercial)
- multi-platform (Windows 95/98/ME/NT/2K/XP/2003, Windows CE, Linux, Unix)
- loading and saving of INI-style configuration files
- configuration files can have any newline format on all platforms
- liberal acceptance of file format
  * key/values with no section
  * removal of whitespace around sections, keys and values
- support for multi-line values (values with embedded newline characters)
- optional support for multiple keys with the same name
- optional case-insensitive sections and keys (for ASCII characters only)
- saves files with sections and keys in the same order as they were loaded
- preserves comments on the file, section and keys where possible.
- supports both char or wchar_t programming interfaces
- supports both MBCS (system locale) and UTF-8 file encodings
- system locale does not need to be UTF-8 on Linux/Unix to load UTF-8 file
- support for non-ASCII characters in section, keys, values and comments
- support for non-standard character types or file encodings via user-written converter classes
- support for adding/modifying values programmatically
- compiles cleanly in the following compilers:
  * Windows/VC6 (warning level 3)
  * Windows/VC.NET 2003 (warning level 4)
  * Windows/VC 2005 (warning level 4)
  * Linux/gcc (-Wall)
  * Windows/MinGW GCC

# Documentation

Full documentation of the interface is available in doxygen format.

# Examples

These snippets are included with the distribution in the file snippets.cpp.

### SIMPLE USAGE

```c++
CSimpleIniA ini;
ini.SetUnicode();
ini.LoadFile("myfile.ini");
const char * pVal = ini.GetValue("section", "key", "default");
ini.SetValue("section", "key", "newvalue");
```

### LOADING DATA

```c++
// load from a data file
CSimpleIniA ini(a_bIsUtf8, a_bUseMultiKey, a_bUseMultiLine);
SI_Error rc = ini.LoadFile(a_pszFile);
if (rc < 0) return false;

// load from a string
std::string strData;
rc = ini.LoadData(strData.c_str(), strData.size());
if (rc < 0) return false;
```

### GETTING SECTIONS AND KEYS

```c++
// get all sections
CSimpleIniA::TNamesDepend sections;
ini.GetAllSections(sections);

// get all keys in a section
CSimpleIniA::TNamesDepend keys;
ini.GetAllKeys("section-name", keys);
```

### GETTING VALUES

```c++
// get the value of a key
const char * pszValue = ini.GetValue("section-name", 
    "key-name", NULL /*default*/);

// get the value of a key which may have multiple 
// values. If bHasMultipleValues is true, then just 
// one value has been returned
bool bHasMultipleValues;
pszValue = ini.GetValue("section-name", "key-name", 
    NULL /*default*/, &amp;bHasMultipleValues);

// get all values of a key with multiple values
CSimpleIniA::TNamesDepend values;
ini.GetAllValues("section-name", "key-name", values);

// sort the values into the original load order
values.sort(CSimpleIniA::Entry::LoadOrder());

// output all of the items
CSimpleIniA::TNamesDepend::const_iterator i;
for (i = values.begin(); i != values.end(); ++i) { 
    printf("key-name = '%s'\n", i->pItem);
}
```

### MODIFYING DATA

```c++
// adding a new section
rc = ini.SetValue("new-section", NULL, NULL);
if (rc < 0) return false;
printf("section: %s\n", rc == SI_INSERTED ? 
    "inserted" : "updated");

// adding a new key ("new-section" will be added 
// automatically if it doesn't already exist)
rc = ini.SetValue("new-section", "new-key", "value");
if (rc < 0) return false;
printf("key: %s\n", rc == SI_INSERTED ? 
    "inserted" : "updated");

// changing the value of a key
rc = ini.SetValue("section", "key", "updated-value");
if (rc < 0) return false;
printf("key: %s\n", rc == SI_INSERTED ? 
    "inserted" : "updated");
```

### DELETING DATA

```c++
// deleting a key from a section. Optionally the entire
// section may be deleted if it is now empty.
ini.Delete("section-name", "key-name", 
    true /*delete the section if empty*/);

// deleting an entire section and all keys in it
ini.Delete("section-name", NULL);
```

### SAVING DATA

```c++
// save the data to a string
rc = ini.Save(strData);
if (rc < 0) return false;

// save the data back to the file
rc = ini.SaveFile(a_pszFile);
if (rc < 0) return false;
```
