# Qt5.2+ project for rpcs3. Works on Windows, Linux and Mac OSX
QT += gui opengl quick widgets
CONFIG += c++11

TARGET = rpcs3-qt

# Qt UI
SOURCES += $$P/rpcs3qt/*.cpp
HEADERS += $$P/rpcs3qt/*.h

# RPCS3
HEADERS += $$P/stdafx.h
INCLUDEPATH += $$P $$P/..
DEFINES += QT_UI

# Installation path
# target.path =

# This line is needed for Qt 5.5 or higher
win32:LIBS += opengl32.lib
