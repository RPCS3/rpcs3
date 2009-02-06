; macros.inc -  description
; -------------------
; begin                : Sun Nov 08 2001
; based on ZSNES macros.mac
; email                : linuzappz@pcsx.net

; This program is free software; you can redistribute it and/or modify  *
; it under the terms of the GNU General Public License as published by  *
; the Free Software Foundation; either version 2 of the License, or     *
; (at your option) any later version. See also the license.txt file for *
; additional informations.                                              *


%ifdef __WIN32__

%imacro EXTSYM 1-*
%rep %0
  	extern _%1
  	%define %1 _%1
%rotate 1
%endrep
%endmacro

%imacro NEWSYM 1
  global _%1
  _%1:
  %1:
%endmacro

%else

%define EXTSYM extern

%imacro NEWSYM 1
	global %1
	%1:
%endmacro

%endif

