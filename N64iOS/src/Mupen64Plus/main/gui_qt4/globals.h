/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - globals.h                                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Slougi                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <QStringList>
#include <QIcon>

const int BUF_MAX = 512;
const QStringList RomExtensions = QStringList() << "*.rom"
                                                << "*.v64"
                                                << "*.z64"
                                                << "*.gz"
                                                << "*.zip"
                                                << "*.n64"
                                                << "*.bz2"
                                                << "*.lzma"
                                                << "*.7z";
QStringList romDirectories();
QIcon icon(QString iconName);
QPixmap pixmap(QString name, QString subdirectory = QString());
void drawStars(QPainter* painter,
               const QRect& rect,
               int n,
               int m);

#endif // __GLOBALS_H__

