#***************************************************************************
#                             Glide64 Makefile
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
USES_QT4 = true
USES_GTK2 = true
include ../pre.mk

# local CFLAGS, LIBS, and LDFLAGS
CFLAGS += $(SDL_FLAGS) $(GTK_FLAGS) -fpic -DPIC -DGCC -DUSE_GTK -Iwrapper/
LDFLAGS += $(PLUGIN_LDFLAGS) $(LIBGL_LIBS) $(SDL_LIBS)

ifeq ($(VPDEBUG), 1)
  CFLAGS  += -DVPDEBUG
  LDFLAGS += -lIL
endif
ifeq ($(RDP_LOG_ERR), 1)
  CFLAGS += -DRDP_LOGGING -DRDP_ERROR_LOG
else
  ifeq ($(RDP_LOG), 1)
    CFLAGS += -DRDP_LOGGING
  endif
endif

# list of object files to generate
OBJECTS = \
	Main.o \
	rdp.o \
	Ini.o \
	TexCache.o \
	Debugger.o \
	Util.o \
	CRC.o \
	Combine.o \
	TexBuffer.o \
	3dmath.o \
	DepthBufferRender.o

WRAPPEROBJECTS = \
	wrapper/combiner.o \
	wrapper/textures.o \
	wrapper/main.o \
	wrapper/geometry.o \
	wrapper/config.o \
	wrapper/filter.o \
	wrapper/2xsai.o \
	wrapper/hq2x.o \
	wrapper/hq4x.o

GTKOBJECTS = \
	support_gtk2.o \
	messagebox_gtk2.o \
	Config_gtk2.o

QT_OBJECTS = \
	messagebox_qt4.o \
	config_qt4.o \
	configdialog_qt4.o

MOC_OBJECTS = \
	configdialog_qt4.moc

OBJECTS	+= $(WRAPPEROBJECTS)
TARGET	= glide64.so

CFLAGS += -MMD -MP -MQ $@

ifeq ($(GUI), QT4)
  LDFLAGS += $(QT_LIBS)
  CFLAGS += $(QT_FLAGS) -DQT_NO_STL
  OBJECTS += $(QT_OBJECTS)
  QT_EXTRA := $(MOC_OBJECTS) ui_glide64config.h
else
  LDFLAGS += $(GTK_LIBS)
  CFLAGS += -DUSE_GTK $(GTK_FLAGS)
  OBJECTS += $(GTKOBJECTS)
endif

DEPS := $(OBJECTS:.o=.d) $(QT_OBJECTS:.o=.d)
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

# build targets
targets:
	@echo "Glide64 Video Plugin + wrapper makefile. "
	@echo "  Targets:"
	@echo "    all        == Build Glide64 video plugin"
	@echo "    clean      == remove object files"
	@echo "    rebuild    == clean and re-build all"
	@echo "  Options:"
	@echo "    BITS=32       == build 32-bit binaries on 64-bit machine"
	@echo "    PROFILE=1     == build gprof instrumentation into binaries for profiling"
	@echo "    DBGSYM=1      == add debugging symbols to binaries"
	@echo "    NO_ASM=1      == disable inline assembly language code"
	@echo "    VPDEBUG=1     == some kind of debugging"
	@echo "    RDP_LOG=1     == Dump out RDP instruction data"
	@echo "    RDP_LOG_ERR=1 == Dump out RDP instruction data + errors"
	@echo "    GUI=GTK2      == Build gtk+-2 GUI"
	@echo "    GUI=QT4       == Build Qt4 GUI"

all: $(TARGET)

$(TARGET): $(QT_EXTRA) $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
	$(STRIP) $@

ui_glide64config.h: glide64config.ui
	$(UIC) $< -o $@

configdialog_qt4.moc: configdialog_qt4.h
	$(MOC) $< -o $@

Main.o: font.h cursor.h
font.h:	compiletex
	./compiletex font.tex font.h font

cursor.h: compiletex
	./compiletex cursor.tex cursor.h cursor

compiletex: compiletex.c
	@rm -f compiletex compiletex.o
	$(CC) -o compiletex.o -c $<
	$(LD) -o $@ compiletex.o

rdp.o: ucode06.h

clean:
	$(RM) -rf $(OBJECTS) $(TARGET) $(DEPS) compiletex compiletex.o font.h cursor.h
	$(RM) -rf *.ii *.i *.s *.moc ui_*.h

rebuild: clean $(TARGET)


# build rules
.SUFFIXES: .o .cpp .c
.cpp.o:
# in case this was invoked because a header changed and the compilation fails,
# the record of dependency in the .d is gone, but not the .o
	@rm -f $@
	$(CXX) -o $@ $(CFLAGS) -c $<

.c.o:
	@rm -f $@
	$(CC) -o $@ $(CFLAGS) -c $<

.PHONY: all clean rebuild

