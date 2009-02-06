PCSX2 is a PlayStation 2 emulator for Windows and Linux, started by the same team that brought you PCSX (a Sony PlayStation 1 emulator).

The PCSX2 project attempts to allow PS2 code to be executed on your computer, thus meaning you can put a PS2 DVD or CD into your computers drive, and boot it up!


Overview

The project has been running for more than five years now, and since it's initial release has grown in compatibility. From initially just being able to run a few public domain demos, it's current state enables many games to boot and actually go in game, such as the 'famous' Final Fantasy X or Devil May Cry 3. You can always visit the PCSX2 homepage (http://www.pcsx2.net) to check the latest compatibility status of games with more than 1800 titles tested.


Configuration

A very detailed guide is available on the PCSX2 homepage which is already translated in several languages!
You can consult it here: http://www.pcsx2.net/guide.php


Status

PCSX2 has come a long way since its’ starting point back at 2002.Current features include:

•Separate recompilers for Emotion Engine (EE) , Vector Unit 0 (VU0) and Vector Unit 1 (VU1).
•Dual core support, with the Graphics Synthesizer (GS) running on a second thread.
•Usage of MMX,SSE1,SSE2 and limited SSE3 extensions
•Proper SPU2 emulation featuring Auto DMA and Time Scaling
•Full gamepad support featuring Dual Shock 2,analog controls and even supporting analog movement over keyboard (using some external plugins)
•Patch system for creating cheats and for workarounds on games

Sections that still need work:

•Dev9 functions, such as HDD and Ethernet (partially implemented) support
•Firewire emulation (quite low on the list though)
•USB emulation (very partially implemented)
•Image Processing Unit (IPU) emulation which is responsible for the FMV playback.It has been implemented but it is buggy and slow
•MIPS cache needs to be properly implemented (barely works at this time)




How can you help

As most of you are aware, the PCSX2 team is working on this project at the expense of their free time and provide it without charging the program’s use.
If you want to show your appreciation to these people and motivate them, you can donate any amount of money you feel is right to the team’s paypal account found in the official site.
These funds will be used so the team members can get more modern and new hardware in order to test and debug more efficiently and even implement new features (just like dual core support for example).

If you are a programmer and you are interested in helping the PCSX2 team by making additions or corrections to the code, you are free to browse through the public SVN repository here (http://sourceforge.net/projects/pcsx2) after taking into account PCSX2 is under the GNU General Public Licence (GPL) v2.



The Coding Team

Below you can see 3 tables, showing the current team members who are actively coding at the present time, the current team members who have been inactive for some time and the older team members who for some reason quit along the way, which include the previous project leader Linuzappz to whom we send our best regards :) 


Current active team members:

Nickname 	Real Name           	Place	     Occupation	   Comments 

florin	        Florin Sasu	        Romania	     co-coder	   Master of HLE. Master of cd code and bios HLE..
saqib		     -                  Pakistan     Main Coder	   Fixing bugs around (FPU, Interpreter, VUs...)
Nachnbrenner	     -	                Germany      co-coder	   patch freak :P
aumatt		                        Australia    co-coder	   a bit of everything mostly handles CDVD cmds
refraction	Alex Brown          	England	     co-coder	   General Coding DMA/VIF etc
zerofrog	     -	                USA	     co-coder	   Recompilers, ZeroGS, x86-64, linux, optimizations, general 								   fixes and new features

Current inactive team members:

Nickname 	Real Name           	Place	       Occupation   Comments 

Shadow	        George Moralis      	Greece	       co-coder	    Master of cpu, master of bugs, general coding…
Goldfinger	     -                  Brazil	       co-coder	    MMI,FPU  and general stuff
loser	             -	                Australia      co-coder	    obscure cdvd related stuff

Ex team members:

Nickname 	Real Name           	Place	      Occupation     Comments 

Linuzappz 	     -                  Argentina      Main Coder    Master of The GS emulation and so many others..
basara		     -	                    -          co-coder	     Recompiler programmer. general coding
[TyRaNiD]	     -		            -          co-coder	     GS programmer.General coding
Roor		     -	                    -          co-coder	     General coding

Additional coding: 
F|RES, Pofis, Gigaherz, Nocomp, _Riff_, fumofumo
 
The Beta Tester Team

Beta testers are people (slaves/mindless grunts :P) who constantly test new PCSX2 beta builds to report any new bugs, regressions or improvements. While this might sound simple to most, what many people do not know is that testers also debug with the coders, maintain the huge game compatibility list, create dumps and logs for the coders and so much more. As above, active, inactive and ex members are listed alphabetically

Current active members:

Bositman, CKemu, Crushtest, Falcon4Ever, GeneralPlot, Prafull, RPGWizard, RudyX, Parotaku


Current inactive team members:

Belmont, Knuckles, Raziel


Ex team members:
Chaoscode, CpUMasteR, EFX , Elly, JegHegy, Razorblade, Seta San, Snake875


Additional thanks and credits

Duke of NAPALM: 	     For the 3d stars demo. The first demo that worked in pcsx2 :)
Tony Saveski (dreamtime):    For his great ps2tutorials!!
F|res: 			     Author of dolphin, a big thanks from shadow..
Now3d:			     The guy that helped shadow at his first steps..
Keith:		     	     Who believed in us..
Bobbi & Thorgal: 	     For hosting us, for the old page design and so many other things 
Sjeep:			     Help and info
BGnome:     	             Help testing stuff
Dixon:      		     Design of the old pcsx2 page, and the pcsx2.net domain
Bositman:		     PCSX2 beta tester :)  (gia sou bositman pare ta credits sou )
No-Reccess: 	             Nice guy and great demo coder :) 
NSX2 team:  		     For their help with VU ;)
Razorblade:                  For the old PCSX2 logo & icon.
Snake:      	             He knows what for :P
Ector:       		     Awesome emu :)
Zezu:       		     A good guy. Good luck with your emu  :P
Hiryu & Sjeep:		     For their libcdvd (iso parsing and filesystem driver code)
Sjeep:  	             For the SjDATA filesystem driver
F|res:			     For the original DECI2 implementation
libmpeg2:  		     For the mpeg2 decoding routines
Aumatt:    		     For applying fixes to pcsx2
Microsoft:  		     For VC.Net 2003 (and now 2005) (really faster than vc6) :P
NASM team:                   For nasm
CKemu:   		     Logos/design

and probably to a few more..

Special Shadow's thanks go to...

My friends: Dimitris, James, Thodoris, Thanasis and probably to a few more..and of course to a lady somewhere out there....


Created for v0.9.4 by bositman.







The PCSX2 Coding and Beta testing team

