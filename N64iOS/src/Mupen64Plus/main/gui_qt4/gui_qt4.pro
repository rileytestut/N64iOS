#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# *   Mupen64plus - gui_qt4.pro                                             *
# *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
# *   Copyright (C) 2008 Slougi                                             *
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

TEMPLATE = lib
CONFIG += staticlib release
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += gtk+-2.0 sdl
}

# Input
HEADERS += globals.h \
           mainwidget.h \
           mainwindow.h \
           romdelegate.h \
           romdirectorieslistwidget.h \
           rominfodialog.h \
           rommodel.h \
           settingsdialog.h \
           starlabel.h \
           ../config.h \
           ../main.h \
           ../version.h \
           ../romcache.h \
           ../md5.h \
           ../plugin.h \
           ../winlnxdefs.h \
           ../util.h \
           ../savestates.h \
           ../rom.h \
           ../7zip/7zMain.h \
           ../7zip/7zIn.h \
           ../7zip/7zHeader.h \
           ../7zip/Types.h \
           ../7zip/7zItem.h \
           ../7zip/7zMethodID.h \
           ../7zip/7zBuffer.h \
           ../7zip/7zAlloc.h
FORMS += mainwidget.ui \
         mainwindow.ui \
         romdirectorieslistwidget.ui \
         rominfodialog.ui \
         settingsdialog.ui
SOURCES += globals.cpp \
           main.cpp \
           mainwidget.cpp \
           mainwindow.cpp \
           romdelegate.cpp \
           romdirectorieslistwidget.cpp \
           rominfodialog.cpp \
           rommodel.cpp \
           settingsdialog.cpp \
           starlabel.cpp \
           translate.cpp
