mintty
=======================
This is a modified version of the mintty terminal emulator program for cygwin and msys.

http://code.google.com/p/mintty/

Here are some of the changes:
  * stubbed out win calls
  * allowed headers to be included within C++ projects.
    * renamed variables, typedefs & macros which use C++ reserved words. (class, delete, new, or)
    * renamed some typedefs / macros to not confliced with existing C++ types. (string)
  * commented or ifdef'ed out windows dependencies.
