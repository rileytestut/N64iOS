/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - mainwindow.cpp                                          *
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

#include <QCoreApplication>
#include <QMap>

#include "../translate.h"

// We need the pointers to remain valid after the tr call returns, so cache the
// translations
static QMap<QString, QString> translations;

extern "C" {
    void tr_init(void) {}
    void tr_delete_languages(void) {}
    list_t tr_language_list(void) { return 0; }
    int tr_set_language(const char *name) { Q_UNUSED(name); return 0; }
    const char *tr(const char *text) {
        // update the translation every time, it might have changed
        translations[text] = QCoreApplication::translate("", text);
        return qPrintable(translations[text]);
    }
}
