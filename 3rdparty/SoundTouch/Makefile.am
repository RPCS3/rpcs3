## Process this file with automake to create Makefile.in
##
## $Id: Makefile.am,v 1.3 2006/02/05 18:33:34 Olli Exp $
##
## Copyright (C) 2003 - David W. Durham
##
## This file is part of SoundTouch, an audio processing library for pitch/time adjustments
##
## SoundTouch is free software; you can redistribute it and/or modify it under the
## terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
##
## SoundTouch is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
## A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place - Suite 330, Boston, MA  02111-1307, USA

AUTOMAKE_OPTIONS = foreign

noinst_HEADERS=AAFilter.h cpu_detect.h FIRFilter.h RateTransposer.h TDStretch.h cpu_detect_x86_gcc.cpp
noinst_LIBRARIES = libSoundTouch.a

libSoundTouch_a_CXXFLAGS = -msse -mmmx
libSoundTouch_a_CFLAGS = -msse -mmmx

#lib_LTLIBRARIES=libSoundTouch.la
# the mmx_gcc.cpp and cpu_detect_x86_gcc.cpp may need to be conditionally included here from things discovered in configure.ac
libSoundTouch_a_SOURCES=AAFilter.cpp FIRFilter.cpp FIFOSampleBuffer.cpp mmx_optimized.cpp sse_optimized.cpp \
RateTransposer.cpp SoundTouch.cpp TDStretch.cpp WavFile.cpp cpu_detect_x86_gcc.cpp

# ??? test for -fcheck-new in configure.ac
# other compiler flags to add
AM_CXXFLAGS=-O3 -msse -fcheck-new
#-I../../include

# other linking flags to add
#libSoundTouch_la_LIBADD=

