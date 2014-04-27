
Decriptions of Provided .vsprops Sheets
---------------------------------------

 * plugin_svnroot - Provides a set of semi-standard user macros for plugins that
   conform to an expected folder layout.  Each user macro can be optionally overridden
   by the plugin using its own property sheet, if needed.
   
   See the contents of plugin_svnroot for explanations of the User Macros used by all
   other properties sheets lested below.
   

 * 3rdPartyDeps - Adds the /deps folder to the linker search path.  Does not add
   any actual dependencies.  You must add those manually.

 * pthreads - Adds the w32pthreads library to your project, along with the expected
   compiler defines for correctly compiling and linking pthreads.

 * BaseProperties - Sets up standard Output and Intermediate directories, warning levels,
   struct alignment, and other settings required for Pcsx2 and its libs to link in
   a workable fashion.  Adds standard preprocessor defines for:
   __WIN32__;WIN32;_WINDOWS;_CRT_SECURE_NO_WARNINGS;_CRT_SECURE_NO_DEPRECATE
   
 * IncrementalLinking - Enables incremental linking, for use in devel/debug modes only.
   Incremental linking force-disables Whole Program Optimization, but builds the result
   .exe/.dll much quicker usually.

 * GlobalLinking - Enables full support for Whole Program Optimization, and force-
   disables any conflicting incremental link settings.
   