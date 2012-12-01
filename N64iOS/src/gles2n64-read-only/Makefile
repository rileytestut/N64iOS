#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# *   Mupen64plus - Makefile                                                *
# *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
# *   Copyright (C) 2007-2008 DarkJeztr Tillin9 Richard42                   *
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
# Makefile for glN64 in Mupen64plus.

ARCH = ARM
OS = LINUX

#SO_EXTENSION = dll
#CXX = 	C:/MinGW/bin/g++
#LD = 	C:/MinGW/bin/g++
#STRIP = C:/MinGW/bin/strip

SO_EXTENSION = dylib

VERSION = 6.0
JAILBREAKDIR = .
LLVMFILE = llvm-
TOOLVER = 4.2

CFLAGS = -F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${VERSION}.sdk/System/Library/Frameworks -F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${VERSION}.sdk/System/Library/PrivateFrameworks -I./ -I../../ -I../../Ads -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${VERSION}.sdk -L/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${VERSION}.sdk/usr/lib -L../libpng/png/build/Debug-iphoneos -L../libpng/png/build/Release-iphoneos -L../../../iN64/build/Debug-iphoneos -L../../../iN64/build/Release-iphoneos -L../SDL/Xcode-iPhoneOS/SDL/build/Debug-iphoneos -L../SDL/Xcode-iPhoneOS/SDL/build/Release-iphoneos
CFLAGS += -I../../include/ -I../../include/SDL -I../SDL/include -I../SDL/ -I../SDL/src/ -I../SDL/src/events -I../SDL/src/video -I../SDL/src/video/uikit -I../libpng/ -I./EGL_mac_ios/include

SDL_FLAGS =
SDL_LIBS = 

LDFLAGS = -lobjc \
    	      -lpthread \
              -framework CoreFoundation \
              -framework Foundation \
              -framework UIKit \
              -framework QuartzCore \
              -framework CoreGraphics \
              -framework CoreSurface \
              -framework CoreLocation \
              -framework AudioToolbox \
              -framework GraphicsServices \
              -framework OpenGLES \
              -lz \
              -lpng \
              -lSDLiPhoneOS

CXX     = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++
LD      = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++
STRIP	= /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip -x
AR	    = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ar

CFLAGS  += -Wall -pipe -O3

DRIVE 	= 
# -march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -ftree-vectorize
CFLAGS  += -DARM_ARCH -arch armv7 -ffast-math \
		  -fsingle-precision-constant  -funroll-loops \
		  -fexpensive-optimizations -fomit-frame-pointer

#CFLAGS  += -findirect-inlining
#CFLAGS  += -ftree-switch-conversion
#CFLAGS  += -floop-interchange -floop-strip-mine -floop-block
CFLAGS += -miphoneos-version-min=3.0 -I. -O3 -DPIC
CFLAGS += -ffast-math -fexpensive-optimizations -ftemplate-depth-36 -mstructure-size-boundary=32 -falign-functions=32 -falign-loops -falign-labels -falign-jumps -fno-builtin -fno-common -fomit-frame-pointer -fno-strength-reduce -fno-strict-aliasing

#CFLAGS  += -D__NEON_OPT -D__VEC4_OPT -D__PACKVERTEX_OPT

CFLAGS += -Wall -D__LINUX__ -fPIC

#CFLAGS += -save-temps
#CFLAGS += -DPROFILE_GBI

CFLAGS += -x objective-c++ #Allows compilation of objective-C code as well

# list of object files to generate
OBJECTS = Config.o \
  gles2N64.o \
  OpenGL.o \
  N64.o \
  RSP.o \
  VI.o \
  Textures.o \
  ShaderCombiner.o \
  gDP.o \
  gSP.o \
  GBI.o \
  DepthBuffer.o \
  CRC.o \
  2xSAI.o \
  RDP.o \
  F3D.o \
  F3DEX.o \
  F3DEX2.o \
  L3D.o \
  L3DEX.o \
  L3DEX2.o \
  S2DEX.o \
  S2DEX2.o \
  F3DPD.o \
  F3DDKR.o \
  F3DWRUS.o \
  ./EGL_mac_ios/src/egl.o \
  #../SDL/src/SDL_compat.o \
  #../SDL/src/video/SDL_video.o \
  #../SDL/src/audio/SDL_audio.o \
  #../SDL/src/audio/SDL_audiocvt.o \
  #../SDL/src/audio/SDL_audiotypecvt.o \
  #../SDL/src/events/SDL_events.o \
  #../SDL/src/events/SDL_mouse.o

# build targets
all: gles2n64.$(SO_EXTENSION)

clean:
	rm -f *.o ./wes/*.o *.$(SO_EXTENSION) ui_gln64config.* ../Mupen64Plus/bin/gles2n64.$(SO_EXTENSION)

# build rules
.cpp.o:
	$(CXX) -o $@ $(CFLAGS) -c $<

.c.o:
	$(CXX) -o $@ $(CFLAGS) -c $<

ui_gln64config.h: gln64config.ui
	$(UIC) $< -o $@

gles2n64.$(SO_EXTENSION): $(OBJECTS)
	$(CXX) $^ $(CFLAGS) $(LDFLAGS) $(ASM_LDFLAGS) $(PLUGIN_LDFLAGS) $(SDL_LIBS) $(LIBGL_LIBS) -undefined dynamic_lookup -dynamiclib -install_name /Applications/n64ios.app/plugins/$@ -o $@
	cp -f $@ ../Mupen64Plus/bin/
	#ldid -S ../Mupen64Plus/bin/$@
    
    # -undefined dynamic_lookup is magic

gles2N64.o: gles2N64.cpp
	$(CXX) $(CFLAGS) $(SDL_FLAGS) -DMAINDEF -c -o $@ $<

Config_qt4.o: Config_qt4.cpp ui_gln64config.h
	$(CXX) $(CFLAGS) $(SDL_FLAGS) -c -o $@ $<

Config_gtk2.o: Config_gtk2.cpp
	$(CXX) $(CFLAGS) $(SDL_FLAGS) -c -o $@ $<

GBI.o: GBI.cpp
	$(CXX) $(CFLAGS) $(SDL_FLAGS) -c -o $@ $<

