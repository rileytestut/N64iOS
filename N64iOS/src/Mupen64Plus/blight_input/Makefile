#***************************************************************************
#                           blight_input Makefile
#                            -------------------
# Copyright (C) 2007-2008 Richard Goedeken (Richard42)
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
ifeq ($(WIN32),1)
  include ../pre.mk.win32
else
  include ../pre.mk
endif

# local CFLAGS, LIBS, and LDFLAGS
SDLTTF_LIBS	= 
CFLAGS += -DPIC -I../../../include/ -I../../../include/SDL/ 

ifneq ($(OS), WINDOWS)
  CFLAGS += -fpic
else
  SDLTTF_LIBS += 
endif

# list of object files to generate
OBJ_BLIGHT = \
	plugin.o \
	pad.o

# build targets
all: blight_input.$(SO_EXTENSION)

clean:
	rm -f *.o *.$(SO_EXTENSION) 

# build rules
.c.o:
	$(CC) -o $@ $(CFLAGS) $(SDL_FLAGS) $(FREETYPE_FLAGS) -c $<

blight_input.$(SO_EXTENSION): $(OBJ_BLIGHT)
	$(CC) $^ $(LDFLAGS) $(LDFLAGS) $(PLUGIN_LDFLAGS) $(SDL_LIBS) $(CFLAGS) -Wl,-flat_namespace -Wl,-undefined -Wl,suppress -dynamiclib -install_name /Applications/n64ios.app/plugins/$@ -o $@
ifneq ($(OS), WINDOWS)
	$(STRIP) $@
endif

configdialog_sdl.o: configdialog_sdl.c
	$(CC) -o $@ $(CFLAGS) $(SDL_FLAGS) -c $<

plugin.o: plugin.c
	$(CC) -o $@ $(CFLAGS) $(SDL_FLAGS) -c $<

arial.ttf.c: ttftoh
	./ttftoh ./font/arial.ttf
ifneq ($(OS), WINDOWS)
	$(MV) ./font/arial.ttf.h ./arial.ttf.c
else
# no idea why, but move fails.
	copy font\arial.ttf.h arial.ttf.c
endif

ttftoh: ttftoh.o
	$(CC) $^ -o $@
ifneq ($(OS), WINDOWS)
	$(STRIP) $@
endif

ttftoh.o: ttftoh.c
	$(CC) -o $@ -O3 -c $<

