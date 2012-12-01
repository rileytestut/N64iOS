#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# *   Mupen64plus - Makefile                                                *
# *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
# *   Copyright (C) 2007-2008 Tillin9 Richard42                             *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License as published by  *
# *   the Free Software Foundation; either version 2 of the License, or     *
# *   (at your option) any later version.                                   *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU General Public License     *
# *   along with this program; if not, write to the                         *
# *   Free Software Foundation, Inc.,                                       *
# *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
# Makefile for JTTL SDL Audio plugin in Mupen64plus

# include pre-make file with a bunch of definitions
USES_GTK2 = false
include ../pre.mk

# test for presence of libsamplerate
# warn user
#$(warning No libsamplerate development libraries found.  jttl_audio will be built without Best Quality SINC resampler.)
# clear libsamplerate flags and libraries
SRC_FLAGS	= -I../../../include/ -I../../../include/SDL/
SRC_LIBS	=

#LDFLAGS += -liN64


# local CFLAGS, LIBS, and LDFLAGS
CFLAGS += -fpic -DPIC

CFLAGS += -L../../../../iN64/build/Debug-iphoneos -L../../../../iN64/build/Release-iphoneos

# list of object files to generate
OBJECTS = main.o volume.o

ifneq ($(findstring $(GUI), GTK2 QT4),)
  CFLAGS += $(GTK_FLAGS)
  PLUGIN_LDFLAGS += $(GTK_LIBS)
  OBJECTS += gui_gtk2.o
endif

ifeq ($(GUI),NONE)
  OBJECTS += gui_none.o
endif

# build targets
all: version.h jttl_audio.dylib

clean:
	rm -f *.o *.so ../bin/jttl_audio.dylib

# build rules
version.h: FORCE
	@sed 's|@MUPEN_VERSION@|\"$(MUPEN_VERSION)\"| ; s|@PLUGIN_VERSION@|\"$(PLUGIN_VERSION)\"|' \
        ../main/version.template > version.h
	@$(MV) version.h ../main/version.h

.c.o:
	$(CC) $(CFLAGS) $(SDL_FLAGS) $(SRC_FLAGS) -c -o $@ $<

jttl_audio.dylib: $(OBJECTS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) $(ASM_LDFLAGS) $(PLUGIN_LDFLAGS) $(SDL_LIBS) $(GTK_LIBS) $(GTHREAD_LIBS) $(LIBGL_LIBS) $(CFLAGS) -Wl,-flat_namespace -Wl,-undefined -Wl,suppress -dynamiclib -install_name /Applications/n64ios.app/plugins/$@ -o $@
#	$(STRIP) $@
	cp -f $@ ../bin/
	#ldid -S ../bin/$@

FORCE:

