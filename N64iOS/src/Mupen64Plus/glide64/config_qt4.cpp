/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - config_qt4.cpp                                          *
*   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
*   Copyright (C) 2008 slougi                                             *
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

#include <QDebug>
#include <QString>
#include <string.h>

#include "Gfx1.3.h"
#include "configdialog_qt4.h"

extern "C" {

void CALL DllConfig(HWND hParent)
{
    ReadSettings();

    char name[21] = "DEFAULT";
    ReadSpecialSettings(name);

    if (gfx.HEADER) {
        for (int i = 0; i < 20; i++) {
            name[i] = gfx.HEADER[(32+i)^3];
        }
        name[20] = '\0';

        ReadSpecialSettings(qPrintable(QString(name).trimmed()));
    }

    ConfigDialog cd(QWidget::find(hParent));
    cd.exec();
}

} // extern "C"

