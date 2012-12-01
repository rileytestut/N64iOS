/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main.cpp                                                *
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

#include <QtGlobal>

#ifdef Q_WS_X11
# include <gtk/gtk.h>
#endif

#include <QApplication>
#include <QAbstractEventDispatcher>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>

#include <cstdio>
#include <cstdarg>

#include "rommodel.h"
#include "mainwindow.h"
#include "globals.h"

// ugly globals
static MainWindow* mainWindow = 0;
static QApplication* application = 0;
static QTranslator* translator = 0;

namespace core {
extern "C" {

#include "../version.h"
#include "../main.h"
#include "../config.h"
#include "../gui.h"

// Initializes gui subsystem. Also parses AND REMOVES any gui-specific commandline
// arguments. This is called before mupen64plus parses any of its commandline options.
void gui_init(int *argc, char ***argv)
{
    application = new QApplication(*argc, *argv);

    QString locale = QLocale::system().name();
    QString translation = QString("mupen64plus_%1").arg(locale);
    QString path = QString("%1translations").arg(get_installpath());
    translator = new QTranslator;
    if (!translator->load(translation, path)) {
        qDebug("Could not load translation %s. Looked in %s.",
                  qPrintable(translation), qPrintable(path));
    }

    application->installTranslator(translator);

#ifdef Q_WS_X11
    if (QAbstractEventDispatcher::instance()->inherits("QEventDispatcherGlib")) {
        delete application;
        application = 0;
        gtk_init(argc, argv);
        application = new QApplication(*argc, *argv);
        application->installTranslator(translator);
    } else {
        QMessageBox::warning(0,
            QObject::tr("No Glib Integration"),
            QObject::tr("<html><p>Your Qt library was compiled without glib \
                         mainloop integration. Plugins that use Gtk+ \
                         <b>will</b> crash the emulator!</p>\
                         <p>To fix this, install a Qt version with glib \
                         main loop support. Most distributions provide this \
                         by default.</p></html>"));
    }
#endif

    application->setOrganizationName("Mupen64Plus");
    application->setApplicationName("Mupen64Plus");
    application->setWindowIcon(icon("mupen64plus.png"));

#if QT_VERSION >= 0x040400
    application->setAttribute(Qt::AA_NativeWindows);
#endif

    mainWindow = new MainWindow;
}

// display GUI components to the screen
void gui_display(void)
{
    mainWindow->show();
    QApplication::instance()->processEvents();
    QApplication::instance()->sendPostedEvents();
}

// Give control of thread to the gui
void gui_main_loop(void)
{
    application->exec();
    config_write();
    delete mainWindow;
    mainWindow = 0;
    delete application;
    application = 0;
    delete translator;
    translator = 0;
}

int gui_message(gui_message_t messagetype, const char *format, ...)
{
    if (!gui_enabled())
        return 0;

    va_list ap;
    char buffer[2049];
    int response = 0;

    va_start(ap, format);
    vsnprintf(buffer, 2048, format, ap);
    buffer[2048] = '\0';
    va_end(ap);

    if (messagetype == 0) {
        InfoEvent* e = new InfoEvent;
        e->message = buffer;
        application->postEvent(mainWindow, e);
    } else if (messagetype == 1) {
        AlertEvent* e = new AlertEvent;
        e->message = buffer;
        application->postEvent(mainWindow, e);
    } else if (messagetype == 2) {
        //0 - indicates user selected no
        //1 - indicates user selected yes
        ConfirmEvent e;
        e.message = buffer;
        application->sendEvent(mainWindow, &e);
        response = e.isAccepted();
    }

    return response;
}

void gui_update_rombrowser(unsigned int roms, unsigned short clear)
{
    RomModel::self()->update(roms, clear);
}

void gui_set_state(gui_state_t state)
{
     if (!gui_enabled())
        return;

    mainWindow->setState(state);
}

} // extern "C"
} // namespace core

