riftty
============

Terminal emulator meant for use with the Oculus Rift headseat.

Implementation notes
-----------------------

Don't re-implment terminal emulation. Try to scoop code from some opensource project.

xterm
---------
Not very modular...
would be difficult to refactor Xwindow stuff from charcode handling.

* VTRun() is main pooling function
* much of the logic for parsing charcodes seems to be in charproc.c (which is 10k lines)

iTerm2
-----------
most code is in ObjC.
However, it looks like VT100Terminal and VT100Screen are somewhat modular, and are mostly C like, internally.
I should be able to convert this into C++, or C.

Terminator
-------------
Written in Java. Fail.

rxvt
------------
Code looks ok, basic, It seems to be missing color support tho.

PuTTY
-------------
It looks like the is a clear seperation between backend and front end.
However, PuTTY is designed for remote connections, ssh etc.
I'm not sure if it can be used with local ptys.

mintty
-------------
Is a stripped down version of PuTTY 0.6 for local pty sessions only.
Sounds like exactly what I want, except it's not cross platform, only Cygwin/MSYS on windows.
This might be my best bet, the Win32 main loop matches SDL main loop resonably well.
I know the Win32 api more then Xwindows.  So, I'm pretty confident I can pull this off.
