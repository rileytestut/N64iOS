#***************************************************************************
#                           dummy_input Makefile
#                            -------------------
# Copyright (C) 2007-2008 Scott Gorman (okaygo)
#***************************************************************************

#***************************************************************************
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License as published by  *
#*   the Free Software Foundation; either version 2 of the License, or     *
#*   (at your option) any later version.                                   *
#*                                                                         *
#***************************************************************************


# include pre-make file with a bunch of definitions
include ../pre.mk

# local CFLAGS, LIBS, and LDFLAGS
CFLAGS += -fpic -DPIC -D__LINUX__ -I../../../include/ -I../../../include/SDL/ -I../../SDL/src/events -I../../SDL/src/video
CFLAGS += -L../../../../iN64/build/Debug-iphoneos

#LDFLAGS += -lSDL4iOS

# list of object files to generate
OBJECTS = \
	input.o \
    #../../SDL/src/SDL.o \
    #../../SDL/src/video/SDL_surface.o \
    #../../SDL/src/video/SDL_video.o \
    #../../SDL/src/video/uikit/SDL_uikitappdelegate.o \
    #../../SDL/src/video/uikit/SDL_uikitevents.o \
    #../../SDL/src/video/uikit/SDL_uikitopengles.o \
    #../../SDL/src/video/uikit/SDL_uikitopenglview.o \
    #../../SDL/src/video/uikit/SDL_uikitvideo.o \
    #../../SDL/src/video/uikit/SDL_uikitwindow.o \
    #../../SDL/src/joystick/iphoneos/SDL_sysjoystick.o \
    #../../SDL/src/joystick/iphoneos/SDLUIAccelerationDelegate.o
    

# build targets
all: dummyinput.dylib

clean:
	rm -f *.o *.dylib ../bin/dummyinput.dylib

# build rules
.c.o:
	$(CXX) -o $@ $(CFLAGS) $(SDL_FLAGS) $(GTK_FLAGS) -c $<

dummyinput.dylib: $(OBJECTS)
	$(CXX) $^ $(CFLAGS) $(LDFLAGS) $(ASM_LDFLAGS) $(PLUGIN_LDFLAGS) $(SDL_LIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(LIBGL_LIBS) -Wl,-flat_namespace -Wl,-undefined -Wl,suppress -dynamiclib -install_name /Applications/n64ios.app/plugins/$@ -o $@
#	$(STRIP) $@
	cp -f $@ ../bin/
	#ldid -S ../bin/$@

input.o: input.c
	$(CXX) $(CFLAGS) $(SDL_FLAGS) -DMAINDEF -c -o $@ $<

