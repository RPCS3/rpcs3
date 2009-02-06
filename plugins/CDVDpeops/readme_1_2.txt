P.E.Op.S. PS2 CDVD emulation plugin
---------------------------------------------------------------------------

The P.E.Op.S. PS2 CDVD plugin is based on the P.E.Op.S. PSX 
CDR plugin which is based on Pete's CDR ASPI/IOCTL plugin
for Windows.

---------------------------------------------------------------------------
Introduction - 19.11.2003
----------------------------------------------------------------------------

PS2 emulation is growing. 

Oh, don't get me wrong, it will still need a lot of time until 
you can really play your favourite PS2 games on the PC (ehe... 
and I remember how I got flamed away nearly two years ago by 
die-hard know-it-alls when I predicted that it will need at 
least 'a couple of years' for 'playable' PS2 emulation).

But yeah, it is growing. Lotsa nice guys are spending their 
free time coding on PCSX2, for example. One of them is Shadow...
and he never gets tired to ask me for some PS2 plugins, ehe.

Well, what to say? Last week I had some free time as well,
Shadow asked for a CDVD plugin, and so I got to work. I took 
the P.E.Op.S. cdr sources, added the PCSX2 interface, changed 
some lines of code, asked a few stupid questions (hi to 
linuzappz), tested it with a few PS2 dvds and cds, and it was
done.

Of course it's not 100% complete. There are a few (but not 
important) things missing, more cd/dvd modes have to get
investigated and added, etc. But basically I hope it will
work just fine with the current PCSX2 version. So go on,
and give it a try :)

----------------------------------------------------------------------------

Requirements:

* A cdrom/dvd drive (yeah, you need a dvd drive to play dvds...
  no emails please telling me that your cd drive doesn't work
  correctly with dvds). 
* The ASPI layer with W9x/ME
* Nothing special with W2K/WXP
* Some PS2 cds/dvds. 

----------------------------------------------------------------------------

Installation: 

just copy the file cdvdPeops.dll into your PCSX2 \plugins 
directory, that's all.

----------------------------------------------------------------------------

Configuration (similar to the P.E.Op.S. psx cdr plugin):

You HAVE TO configure the plugin before you use
it the first time. There are only a few options
available:

0) Interface
-----------------

If you are using W9x/ME, you have to use the ASPI Interface.
If you are using NT/W2K/XP, you have the free choice:
ASPI (if it's installed), or IOCTL scsi commands.

1) Drive
--------------

Well, that's self-explaining. Just select the drive
you want to use. "NONE" is NO drive... you have to
select a real one.

2.) Caching
-------------------------------------

To get more speed, there are five caching modes:
None, Read ahead, Async, Thread and Smooth.

- 'None' is the slowest mode, but it should work on 
most drives.
- 'Read ahead' will read more sectors at once, speeding up
mdecs. There is a small chance that a few drives cannot
do it, so set it to 'None', if you are having troubles.
- The 'Async mode' will do read ahead and some additional 
'intelligent' asynchronous reads... that mode is recommended
with the ASPI interface, it can squeeze some more speed 
out of your drive :)
- The 'Thread mode' will speed up the IOCTL interfaces,
since that ones can't do real async reading. So, when
you are using W2K/XP, and you have no ASPI installed,
try this mode for best speed.
- Some drives will have speed problems reading ps2 
cds/dvds, this caching mode will try to solve such issues. 


Also available: an additional data cache option, which
will store up to 4 MBytes of already read sectors. This
can speed up certain games, which are re-reading the
same range of sectors again and again.


3.) Speed limitation
-------------------------------

Some drives will work better (less noisy and smoother)
if you limit the drive speed. Not all drives are supporting
the "set speed" command I am using, though.
If your drive doesn't support it, a message will be displayed
on startup. There are some tools in the net which will
offer the same function, prolly for a wider range of drives,
so you can also try one of them, if the plugin speed limit fails.


4.) Don't wait til drive is ready
-----------------------------------------------

By default my plugin is asking the cd/dvd drive on startup,
if its state is ready (that means: a cd is inserted and the 
drive can start reading).
A few drives will not answer that question (bah, bah, bah), 
and the screen will stay black... forever :)
If you are encoutering that problem, you can turn on the 
"Don't wait..." option, but be warned: it can cause problems
(blue screens, for example) if the plugin starts reading and
there is a problem with the drive...


5.) Check drive tray state
-----------------------------------------------
PCSX2 may ask the plugin if the tray is open or closed. If
this option is turned off, the plugion always will respond
"closed". If this option is enabled, the plugin will try
to ask the drive for the tray state. Since I couldn't test
this option yet, I suggest to leave it off (and honestly,
are you able to run a multi-dvd game which needs disc
changing right now in PCSX2?) :)


6.) Try again on reading error
-----------------------------------------------
It might happen that your drive can't read a certain sector at
the first time, if your cd/dvd is scratched. Therefore I've added
that option, by activating it you can tell the plugin to try it
up to 10 times again before reporting the read error to the
main emu.
If you want, you can also activate some error message box,
if a sector is not readable (just to inform you something is
going wrong).


7.) Use PPF patches (not available yet)
---------------------------------------

- TODO :)


8.) Subchannel reading (not available yet)
------------------------------------------

- MAYBE TODO :)

----------------------------------------------------------------------------

Conclusion:

You never ever can escape your Shadow ;)

For version infos read the "version.txt" file.

And, peops, have fun!

Pete Bernert

----------------------------------------------------------------------------

P.E.Op.S. page on sourceforge: https://sourceforge.net/projects/peops/

P.E.Op.S. developer:

Pete Bernert	http://www.pbernert.com
Lewpy			http://lewpy.psxemu.com/
lu_zero			http://brsk.virtualave.net/lu_zero/
linuzappz		http://www.pcsx.net
Darko Matesic	http://mrdario.tripod.com
syo				http://www.geocities.co.jp/SiliconValley-Bay/2072/

----------------------------------------------------------------------------

Disclaimer/Licence:

This plugin is under GPL... check out the license.txt file in the /src
directory for details.

----------------------------------------------------------------------------
