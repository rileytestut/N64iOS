/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - messagebox_qt4.cpp                                      *
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

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "icons/messagebox-error.xpm"
#include "icons/messagebox-info.xpm"
#include "icons/messagebox-quest.xpm"
#include "icons/messagebox-warn.xpm"

#include "Gfx1.3.h"
#include "messagebox.h"

class PluginGuiQueryEvent : public QEvent
{
    public:
        PluginGuiQueryEvent() : QEvent(Type(1003)) {}
        QString message;
        QString title;
        QImage image;
        int flags;
        QWaitCondition* waitCondition;
        int result;
        WId window;
};

static const int MAX = 2048;

extern "C" {

int messagebox(const char *title, int flags, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX];

    va_start( ap, fmt );
    vsnprintf( buf, MAX, fmt, ap );
    va_end( ap );

    PluginGuiQueryEvent* e = new PluginGuiQueryEvent;
    e->message = buf;
    e->title = title;
    e->flags = flags;
    e->result = -1;
    e->window = gfx.hWnd;

    QMutex m;
    QWaitCondition w;
    e->waitCondition = &w;

    switch( flags & 0x00000F00 )
    {
        case MB_ICONWARNING:
            e->image = QImage(messagebox_warn_xpm);
            break;

        case MB_ICONINFORMATION:
            e->image = QImage(messagebox_info_xpm);
            break;

        case MB_ICONQUESTION:
            e->image = QImage(messagebox_quest_xpm);
            break;

        case MB_ICONERROR:
            e->image = QImage(messagebox_error_xpm);
            break;
    }

    QObject* target = 0;
    foreach (QWidget* w, qApp->topLevelWidgets()) {
        if (w->inherits("MainWindow")) {
            target = w;
            break;
        }
    }

    if (target) {
        qApp->postEvent(target, e);
        m.lock();
        w.wait(&m);
        m.unlock();
    }

    return e->result;
}

} // extern "C"

