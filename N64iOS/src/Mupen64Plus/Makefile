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

# include pre-make file with a bunch of definitions
USES_QT4 = true
USES_GTK2 = true

ifeq ($(WIN32),1)
  include ./pre.mk.win32
else
  include ./pre.mk
endif

ifeq ($(OS), LINUX)
#  LDFLAGS += -Wl,-export-dynamic 
endif

# set executable stack as a linker option for X86 architecture, for dynamic recompiler
ifeq ($(CPU), X86)
  ifeq ($(OS), LINUX)
    LDFLAGS += -z execstack
  endif
endif

# keep memory map roughly consistent between architectures
ifeq ($(OS), LINUX)
  ifeq ($(CPU), X86)
    ifeq ($(ARCH), 64BITS)
      LDFLAGS += -Xlinker -Ttext -Xlinker 0x8000000
    endif
  endif
  ifeq ($(CPU), ARM)
    ifeq ($(ARCH), 32BITS)
#      LDFLAGS += -Xlinker -segaddr -Xlinker .init -Xlinker 0x08000000
    endif
  endif
endif

# set options
ifeq ($(DBG), 1)
  CFLAGS += -DDBG
endif
ifeq ($(DBG_COMPARE), 1)
  CFLAGS += -DCOMPARE_CORE
endif
ifeq ($(DBG_CORE), 1)
  CFLAGS += -DCORE_DBG
endif
ifeq ($(DBG_COUNT), 1)
  CFLAGS += -DCOUNT_INSTR
endif
ifeq ($(DBG_PROFILE), 1)
  CFLAGS += -DPROFILE_R4300
endif
ifeq ($(LIRC), 1)
  CFLAGS += -DWITH_LIRC
endif
ifeq ($(GUI), NONE)
  CFLAGS += -DNO_GUI
else
  ifeq ($(GUI), QT4)
  CFLAGS += $(QT_FLAGS) $(GTK_FLAGS)
  LDFLAGS += $(QT_LIBS)
    ifeq ($(DBG), 1)
      CFLAGS += $(GTK_FLAGS)
    endif
  else
    ifeq ($(GUI), GTK2)
      CFLAGS += $(GTK_FLAGS)
    endif
  endif
endif

# set installation options
ifeq ($(PREFIX),)
  PREFIX := /usr/local
endif
ifeq ($(SHAREDIR),)
  SHAREDIR := $(PREFIX)/share/mupen64plus
endif
ifeq ($(BINDIR),)
  BINDIR := $(PREFIX)/bin
endif
ifeq ($(LIBDIR),)
  LIBDIR := $(SHAREDIR)/plugins
endif
ifeq ($(MANDIR),)
  MANDIR := $(PREFIX)/man/man1
endif
ifeq ($(APPLICATIONSDIR),)
  APPLICATIONSDIR := $(PREFIX)/share/applications
endif

INSTALLOPTS := $(PREFIX) $(SHAREDIR) $(BINDIR) $(LIBDIR) $(MANDIR) $(APPLICATIONSDIR)

# list of object files to generate
OBJ_CORE = \
	main/main.o \
	main/romcache.o \
	main/util.o \
	main/cheat.o \
	main/config.o \
	main/adler32.o \
	main/md5.o \
	main/plugin.o \
	main/rom.o \
	main/savestates.o \
	main/zip/ioapi.o \
	main/zip/unzip.o \
	main/bzip2/bzlib.o \
	main/bzip2/crctable.o \
	main/bzip2/decompress.o \
	main/bzip2/compress.o \
	main/bzip2/randtable.o \
	main/bzip2/huffman.o \
	main/bzip2/blocksort.o \
	main/lzma/buffer.o \
	main/lzma/io.o \
	main/lzma/main.o \
	main/7zip/7zAlloc.o \
	main/7zip/7zBuffer.o \
	main/7zip/7zCrc.o \
	main/7zip/7zDecode.o \
	main/7zip/7zExtract.o \
	main/7zip/7zHeader.o \
	main/7zip/7zIn.o \
	main/7zip/7zItem.o \
	main/7zip/7zMain.o \
	main/7zip/LzmaDecode.o \
	main/7zip/BranchX86.o \
	main/7zip/BranchX86_2.o \
	memory/dma.o \
	memory/flashram.o \
	memory/memory.o \
	memory/pif.o \
	memory/tlb.o \
	r4300/r4300.o \
	r4300/bc.o \
	r4300/compare_core.o \
	r4300/cop0.o \
	r4300/cop1.o \
	r4300/cop1_d.o \
	r4300/cop1_l.o \
	r4300/cop1_s.o \
	r4300/cop1_w.o \
	r4300/exception.o \
	r4300/interupt.o \
	r4300/profile.o \
	r4300/pure_interp.o \
	r4300/recomp.o \
	r4300/special.o \
	r4300/regimm.o \
	r4300/tlb.o \
	./../SDL/src/timer/unix/SDL_systimer.o \
	./../SDL/src/timer/SDL_timer.o \
  ./../SDL/src/stdlib/SDL_string.o \
  ./../SDL/src/audio/dummy/SDL_dummyaudio.o \
  ./../SDL/src/audio/SDL_audio.o \
  ./../SDL/src/audio/SDL_audiocvt.o \
  ./../SDL/src/audio/SDL_audiotypecvt.o \
  ./../SDL/src/audio/SDL_mixer.o \
  ./../SDL/src/audio/SDL_mixer_m68k.o \
  ./../SDL/src/audio/SDL_mixer_MMX.o \
  ./../SDL/src/audio/SDL_mixer_MMX_VC.o \
  ./../SDL/src/audio/SDL_wave.o \
  ./../SDL/src/cpuinfo/SDL_cpuinfo.o \
  ./../SDL/src/events/SDL_events.o \
  ./../SDL/src/events/SDL_keyboard.o \
  ./../SDL/src/events/SDL_mouse.o \
  ./../SDL/src/events/SDL_quit.o \
  ./../SDL/src/events/SDL_windowevents.o \
  ./../SDL/src/file/SDL_rwops.o \
  ./../SDL/src/SDL_compat.o \
  ./../SDL/src/SDL_error.o \
  ./../SDL/src/SDL_fatal.o \
  ./../SDL/src/SDL.o \
  ./../SDL/src/thread/pthread/SDL_syscond.o \
  ./../SDL/src/thread/pthread/SDL_sysmutex.o \
  ./../SDL/src/thread/pthread/SDL_syssem.o \
  ./../SDL/src/thread/pthread/SDL_systhread.o \
  ./../SDL/src/thread/SDL_thread.o \
  ./../SDL/src/stdlib/SDL_getenv.o \
  ./../SDL/src/stdlib/SDL_iconv.o \
  ./../SDL/src/stdlib/SDL_malloc.o \
  ./../SDL/src/stdlib/SDL_qsort.o \
  ./../SDL/src/stdlib/SDL_stdlib.o \
  ./../SDL/src/video/SDL_blit.o \
  ./../SDL/src/video/SDL_blit_0.o \
  ./../SDL/src/video/SDL_blit_1.o \
  ./../SDL/src/video/SDL_blit_A.o \
  ./../SDL/src/video/SDL_blit_auto.o \
  ./../SDL/src/video/SDL_blit_copy.o \
  ./../SDL/src/video/SDL_blit_N.o \
  ./../SDL/src/video/SDL_blit_slow.o \
  ./../SDL/src/video/SDL_bmp.o \
  ./../SDL/src/video/SDL_gamma.o \
  ./../SDL/src/video/SDL_pixels.o \
  ./../SDL/src/video/SDL_rect.o \
  ./../SDL/src/video/SDL_renderer_sw.o \
  ./../SDL/src/video/SDL_RLEaccel.o \
  ./../SDL/src/video/SDL_stretch.o \
  ./../SDL/src/video/SDL_surface.o \
  ./../SDL/src/video/SDL_video.o \
  ./../SDL/src/video/SDL_yuv_mmx.o \
  ./../SDL/src/video/SDL_yuv_sw.o \
  ./../SDL/src/video/dummy/SDL_nullevents.o \
  ./../SDL/src/video/dummy/SDL_nullrender.o \
  ./../SDL/src/video/dummy/SDL_nullvideo.o \
  ./../SDL/src/joystick/SDL_joystick.o \
  ./../SDL/src/audio/iphoneos/SDL_coreaudio_iphone.o \
  ./../SDL/src/joystick/iphoneos/SDL_sysjoystick.o \
  ./../SDL/src/joystick/iphoneos/SDLUIAccelerationDelegate.o \
  ./../SDL/src/video/uikit/SDL_uikitevents.o \
  ./../SDL/src/video/uikit/SDL_uikitopengles.o \
  ./../SDL/src/video/uikit/SDL_uikitvideo.o \
  ./../SDL/src/video/uikit/SDL_uikitview.o \
  ./../SDL/src/video/uikit/SDL_uikitwindow.o \
  ./../SDL/src/video/uikit/SDL_uikitopenglview.o \
  ./../SDL/src/video/uikit/SDL_uikitappdelegate.o \
  ./../SDL/src/loadso/dummy/SDL_sysloadso.o \
  ./../SDL/src/video/uikit/jumphack.o \
  ./../SDL/src/video/SDL_renderer_gl.o \
  ./../SDL/src/video/SDL_renderer_gles.o \
  ./../SDL/src/haptic/dummy/SDL_syshaptic.o \
  ./../SDL/src/haptic/SDL_haptic.o \
  ./../SDL/src/loadso/dlopen/SDL_sysloadso.o \
  ./../SDL/src/video/SDL_blendline.o \
  ./../SDL/src/video/SDL_blendpoint.o \
  ./../SDL/src/video/SDL_blendrect.o \
  ./../SDL/src/video/SDL_drawline.o \
  ./../SDL/src/video/SDL_drawpoint.o \
  ./../SDL/src/video/SDL_fillrect.o \
  ./../SDL/src/atomic/dummy/SDL_atomic.o \
  ../../N64iOS/N64iOS/N64DetailViewController.o \
  ../../N64iOS/N64iOS/main.o \
  ../../N64iOS/N64iOS/N64AppDelegate.o \
  #../../main.o \
  ../../NowPlayingController.o \
  ../../helpers.o \
  ../../OptionsController.o \
  ../../RecentController.o \
  ../../RomController.o \
  ../../SaveStatesController.o \
  ../../ShoutOutAppDelegate.o \
  ../../SOApplication.o \
  ../../TabBar.o \
  ../../WebBrowserViewController.o \
  ../../BTDevice.o \
  ../../BTInquiryViewController.o \
  ../../Ads/AltAds.o
	
# handle dynamic recompiler objects
ifneq ($(NO_ASM), 1)
  ifeq ($(CPU), X86)
    ifeq ($(ARCH), 64BITS)
      DYNAREC = x86_64
    else
      DYNAREC = x86
    endif
    OBJ_DYNAREC = \
      r4300/$(DYNAREC)/assemble.o \
      r4300/$(DYNAREC)/debug.o \
      r4300/$(DYNAREC)/gbc.o \
      r4300/$(DYNAREC)/gcop0.o \
      r4300/$(DYNAREC)/gcop1.o \
      r4300/$(DYNAREC)/gcop1_d.o \
      r4300/$(DYNAREC)/gcop1_l.o \
      r4300/$(DYNAREC)/gcop1_s.o \
      r4300/$(DYNAREC)/gcop1_w.o \
      r4300/$(DYNAREC)/gr4300.o \
      r4300/$(DYNAREC)/gregimm.o \
      r4300/$(DYNAREC)/gspecial.o \
      r4300/$(DYNAREC)/gtlb.o \
      r4300/$(DYNAREC)/regcache.o \
      r4300/$(DYNAREC)/rjump.o \
      r4300/new_dynarec/new_dynarec.o \
      r4300/new_dynarec/linkage_$(DYNAREC).o
  else
    ifeq ($(CPU), ARM)
      DYNAREC = arm
    endif
    OBJ_DYNAREC = r4300/empty_dynarec.o \
      r4300/new_dynarec/new_dynarec.o \
      r4300/new_dynarec/linkage_$(DYNAREC).o
  endif
else
  OBJ_DYNAREC = r4300/empty_dynarec.o
endif

OBJ_LIRC = \
	main/lirc.o

OBJ_OPENGL = \
	opengl/OGLFT.o \
	opengl/osd.o \
	opengl/screenshot.o

OBJ_GTK_GUI = \
	main/gui_gtk/main_gtk.o \
        main/gui_gtk/icontheme.o \
	main/gui_gtk/aboutdialog.o \
	main/gui_gtk/cheatdialog.o \
	main/gui_gtk/configdialog.o \
	main/gui_gtk/rombrowser.o \
	main/gui_gtk/romproperties.o

OBJ_DBG = \
	debugger/debugger.o \
	debugger/decoder.o \
	debugger/opprintf.o \
	debugger/memory.o \
	debugger/breakpoints.o

OBJ_GTK_DBG_GUI = \
	main/gui_gtk/debugger/debugger.o \
	main/gui_gtk/debugger/breakpoints.o \
	main/gui_gtk/debugger/desasm.o \
	main/gui_gtk/debugger/memedit.o \
	main/gui_gtk/debugger/varlist.o \
	main/gui_gtk/debugger/registers.o \
	main/gui_gtk/debugger/regGPR.o \
	main/gui_gtk/debugger/regCop0.o \
	main/gui_gtk/debugger/regSpecial.o \
	main/gui_gtk/debugger/regCop1.o \
	main/gui_gtk/debugger/regAI.o \
	main/gui_gtk/debugger/regPI.o \
	main/gui_gtk/debugger/regRI.o \
	main/gui_gtk/debugger/regSI.o \
	main/gui_gtk/debugger/regVI.o \
	main/gui_gtk/debugger/regTLB.o \
	main/gui_gtk/debugger/ui_clist_edit.o \
	main/gui_gtk/debugger/ui_disasm_list.o

OBJ_QT4_GUI = main/gui_qt4/libgui_qt4.a

PLUGINS	= plugins/dummyaudio.$(SO_EXTENSION) \
          plugins/dummyinput.$(SO_EXTENSION) \
          plugins/dummyvideo.$(SO_EXTENSION) \
          plugins/jttl_audio.$(SO_EXTENSION) \
          plugins/mupen64_hle_rsp_azimer.$(SO_EXTENSION) 

#          plugins/blight_input.$(SO_EXTENSION) \
#         plugins/dummyaudio.$(SO_EXTENSION) \
#          plugins/dummyvideo.$(SO_EXTENSION) \
#          plugins/glN64.$(SO_EXTENSION) \
#          plugins/ricevideo.$(SO_EXTENSION) \
#          plugins/jttl_audio.$(SO_EXTENSION) \
#          plugins/mupen64_hle_rsp_azimer.$(SO_EXTENSION) \
#          plugins/mupen64_input.$(SO_EXTENSION) \

#          plugins/glide64.$(SO_EXTENSION)

SHARE = $(shell grep CONFIG_PATH config.h | cut -d '"' -f 2)

# set primary objects and libraries for all outputs
ALL = mupen64plus $(PLUGINS)
OBJECTS = $(OBJ_CORE) $(OBJ_DYNAREC) 
#$(OBJ_OPENGL)
LIBS = $(SDL_LIBS) $(LIBGL_LIBS)

# add extra objects and libraries for selected options
ifeq ($(DBG), 1)
  OBJECTS +=  $(OBJ_DBG) $(OBJ_GTK_DBG_GUI)
  LIBS += -lopcodes -lbfd
endif
ifeq ($(LIRC), 1)
  OBJECTS += $(OBJ_LIRC)
  LDFLAGS += -llirc_client
endif
ifneq ($(GUI), NONE)
  ifneq ($(OS), WINDOWS)
    MISC_DEPS = mupen64plus.desktop
  endif
endif
ifeq ($(GUI), QT4)
  OBJECTS += $(OBJ_QT4_GUI)
  LIBS += $(QT_LIBS) $(GTK_LIBS)
else
  # we reimplement the translation functions in the Qt gui
  OBJECTS += main/translate.o
  ifneq ($(GUI), NONE)
    OBJECTS += $(OBJ_GTK_GUI)
    LIBS += $(GTK_LIBS) $(GTHREAD_LIBS)
  endif
endif

# build targets
targets: version.h $(ALL)

all: version.h $(ALL)

plugins: $(PLUGINS)

mupen64plus: $(MISC_DEPS) version.h $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(CORE_LDFLAGS) $(LIBS) -o $@
	ar rcs libiN64.a $(OBJECTS)
    
ifneq ($(OS), WINDOWS)
#	$(STRIP) $@
	cp -f $@ ./bin/
	#ldid -S ./bin/$@
endif

install:
	./install.sh $(INSTALLOPTS)

uninstall:
	./uninstall.sh $(INSTALLOPTS)

clean-plugins:
ifneq ($(OS), WINDOWS)
	$(MAKE) -C blight_input clean
	$(MAKE) -C dummy_audio clean
	$(MAKE) -C dummy_input clean
	$(MAKE) -C dummy_video clean
	$(MAKE) -C glN64 clean
	$(MAKE) -C rice_video clean
	$(MAKE) -C glide64 clean
	$(MAKE) -C jttl_audio clean
	$(MAKE) -C rsp_hle clean
	$(MAKE) -C mupen64_input clean
	$(RM_F) plugins/mupen64_input.$(SO_EXTENSION) blight_input/arial.ttf.c blight_input/ttftoh plugins/blight_input.$(SO_EXTENSION) plugins/mupen64_hle_rsp_azimer.$(SO_EXTENSION)
	$(RM_F) plugins/dummyaudio.$(SO_EXTENSION) plugins/dummyinput.$(SO_EXTENSION) plugins/dummyvideo.$(SO_EXTENSION) plugins/jttl_audio.$(SO_EXTENSION) plugins/glN64.$(SO_EXTENSION) plugins/ricevideo.$(SO_EXTENSION) plugins/glide64.$(SO_EXTENSION)
endif

clean-core:
ifneq ($(OS), WINDOWS)
	$(RM_F) $(OBJ_CORE)
	$(RM_F) ./r4300/*.o ./r4300/x86/*.o ./r4300/x86_64/*.o ./r4300/new_dynarec/*.o ./memory/*.o ./debugger/*.o ./opengl/*.o
	$(RM_F) ./main/*.o ./main/version.h ./main/zip/*.o ./main/bzip2/*.o ./main/lzma/*.o ./main/7zip/*.o ./main/gui_gtk/*.o ./main/gui_gtk/debugger/*.o
	$(RM_F) mupen64plus mupen64plus.desktop
	$(RM_F) main/gui_qt4/moc_* main/gui_qt4/ui_*.h main/gui_qt4/*.o main/gui_qt4/*.a main/gui_qt4/Makefile
	$(RM_F) translations/*.qm
else
	del /S *.o *.$(SO_EXTENSION) mupen64plus.exe moc_* *.a *.qm
	cd main\gui_qt4
	del /S ui_*.h
endif

clean: clean-core clean-plugins tidy

tidy:
	rm -f libiN64.a

rebuild: clean all

# build rules
mupen64plus.desktop: FORCE
	@sed s:SHARE_DIR:"$(SHAREDIR)": mupen64plus.desktop.in > mupen64plus.desktop

version.h: FORCE
ifneq ($(OS), WINDOWS)
	@sed 's|@MUPEN_VERSION@|\"$(MUPEN_VERSION)\"| ; s|@PLUGIN_VERSION@|\"$(PLUGIN_VERSION)\"|' \
        main/version.template > version.h
	@$(MV) version.h main/version.h
else
	copy version.win32.h main\version.h
endif

r4300/new_dynarec/new_dynarec.o:
	$(CC) -o $@ $(CFLAGS) $(O0_CFLAGS) $(SDL_FLAGS) -c r4300/new_dynarec/new_dynarec.c

.cpp.o:
	$(CXX) -o $@ $(CFLAGS) $(O2_CFLAGS) $(SDL_FLAGS) -c $<

.c.o:
	$(CC) -o $@ $(CFLAGS) $(O2_CFLAGS) $(SDL_FLAGS) -c $<

.m.o:
	$(CC) -o $@ $(CFLAGS) $(O2_CFLAGS) $(SDL_FLAGS) -c $<

.s.o:
	$(CC) -o $@ $(CFLAGS) $(O2_CFLAGS) $(SDL_FLAGS) -c $<
	
main/gui_qt4/Makefile:
	${QMAKE} main/gui_qt4/gui_qt4.pro -o main/gui_qt4/Makefile

main/gui_qt4/libgui_qt4.a: main/gui_qt4/Makefile FORCE
ifneq ($(OS), WINDOWS)
	${MAKE} -C main/gui_qt4
# Run lrelease only on ts files with locale suffix, makes no sense to run it on
# the template. For some reason this fails on windows.
	${LRELEASE} translations/*_*.ts
else
# I wonder whether we can avoid this somehow
	${MAKE} -C main/gui_qt4 CXXFLAGS="${CFLAGS}"
	copy main\gui_qt4\release\libgui_qt4.a main\gui_qt4
endif

plugins/blight_input.$(SO_EXTENSION): FORCE
	$(MAKE) -C blight_input all
ifneq ($(OS), WINDOWS)
#	@$(CP) ./blight_input/blight_input.dylib ./plugins/blight_input.dylib
else
	copy blight_input\blight_input.dll plugins
endif

plugins/dummyaudio.$(SO_EXTENSION): FORCE
	$(MAKE) -C dummy_audio all
ifneq ($(OS), WINDOWS)
#	@$(CP) ./dummy_audio/dummyaudio.dylib ./plugins/dummyaudio.dylib
else
	copy dummy_audio\dummyaudio.dll plugins
endif

plugins/dummyinput.$(SO_EXTENSION): FORCE
	$(MAKE) -C dummy_input all
ifneq ($(OS), WINDOWS)
#	@$(CP) ./dummy_input/dummyinput.dylib ./plugins/dummyinput.dylib
else
	copy dummy_input\dummyinput.dll plugins
endif

plugins/dummyvideo.$(SO_EXTENSION): FORCE
	$(MAKE) -C dummy_video all
ifneq ($(OS), WINDOWS)
#	@$(CP) ./dummy_video/dummyvideo.dylib ./plugins/dummyvideo.dylib
else
	copy dummy_video\dummyvideo.dll plugins
endif

plugins/glN64.$(SO_EXTENSION): FORCE
	$(MAKE) -C glN64 all
#	@$(CP) ./glN64/glN64.$(SO_EXTENSION) ./plugins/glN64.$(SO_EXTENSION)

plugins/ricevideo.$(SO_EXTENSION): FORCE
	$(MAKE) -C rice_video all
#	@$(CP) ./rice_video/ricevideo.$(SO_EXTENSION) ./plugins/ricevideo.$(SO_EXTENSION)

plugins/glide64.$(SO_EXTENSION): FORCE
	$(MAKE) -C glide64 all
#	@$(CP) ./glide64/glide64.$(SO_EXTENSION) ./plugins/glide64.$(SO_EXTENSION)

plugins/jttl_audio.$(SO_EXTENSION): FORCE
	$(MAKE) -C jttl_audio all
#	@$(CP) ./jttl_audio/jttl_audio.$(SO_EXTENSION) ./plugins/jttl_audio.$(SO_EXTENSION)

plugins/mupen64_hle_rsp_azimer.$(SO_EXTENSION): FORCE
	$(MAKE) -C rsp_hle all
#	@$(CP) ./rsp_hle/mupen64_hle_rsp_azimer.$(SO_EXTENSION) ./plugins/mupen64_hle_rsp_azimer.$(SO_EXTENSION)

plugins/mupen64_input.$(SO_EXTENSION): FORCE
	$(MAKE) -C mupen64_input all
#	@$(CP) ./mupen64_input/mupen64_input.$(SO_EXTENSION) ./plugins/mupen64_input.$(SO_EXTENSION)

# This is used to force the plugin builds
FORCE:

