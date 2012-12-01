/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - mainwindow.h                                            *
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

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QEvent>
#include <QActionGroup>
#include <QPointer>

#include "ui_mainwindow.h"

class QLabel;
class QActionGroup;
class QWaitCondition;

extern "C" {
    namespace core {
        #include "../gui.h"
    }
}

enum CustomEventTypes
{
    InfoEventType = QEvent::User,
    AlertEventType,
    ConfirmEventType,
    PluginGuiQueryEventType
};

class MessageEvent : public QEvent
{
    public:
        MessageEvent(Type type) : QEvent(type) {}
        QString message;
};

class InfoEvent : public MessageEvent
{
    public:
        InfoEvent() : MessageEvent(Type(InfoEventType)) {}
};

class AlertEvent : public MessageEvent
{
    public:
        AlertEvent() : MessageEvent(Type(AlertEventType)) {}
};

class ConfirmEvent : public MessageEvent
{
    public:
        ConfirmEvent() : MessageEvent(Type(AlertEventType)) {}
};

class PluginGuiQueryEvent : public QEvent
{
    public:
        PluginGuiQueryEvent() : QEvent(Type(PluginGuiQueryEventType)) {}
        QString message;
        QString title;
        QImage image;
        int flags;
        QWaitCondition* waitCondition;
        int result;
        WId window;
};

// Flags for PluginQueryEvent, from glide64

// The message box contains three push buttons: Abort, Retry, and Ignore.
#define MB_ABORTRETRYIGNORE     (0x00000001)
// Microsoft® Windows® 2000/XP: The message box contains three push buttons:
// Cancel, Try Again, Continue. Use this message box type instead o
// MB_ABORTRETRYIGNORE.
#define MB_CANCELTRYCONTINUE        (0x00000002)
// The message box contains one push button: OK. This is the default.
#define MB_OK               (0x00000004)
// The message box contains two push buttons: OK and Cancel.
#define MB_OKCANCEL         (0x00000008)
// The message box contains two push buttons: Retry and Cancel.
#define MB_RETRYCANCEL          (0x00000010)
// The message box contains two push buttons: Yes and No.
#define MB_YESNO            (0x00000020)
// The message box contains three push buttons: Yes, No, and Cancel.
#define MB_YESNOCANCEL          (0x00000040)

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT
    public:
        MainWindow();
        virtual ~MainWindow();

        void showInfoMessage(const QString& msg);
        void showAlertMessage(const QString& msg);
        void setState(core::gui_state_t state);

    private slots:
        void romOpen();
        void romOpen(const QString& url);
        void romOpen(const QString& url, unsigned int archivefile);
        void romClose();

        void emulationStart();
        void emulationPauseContinue();
        void emulationStop();
        void saveStateSave();
        void saveStateSaveAs();
        void saveStateLoad();
        void saveStateLoadFrom();
        void savestateCheckSlot();
        void savestateSelectSlot(QAction* a);

        void fullScreenToggle();
        void configDialogShow();
        void itemCountUpdate(int count);
        void aboutDialogShow();
        void setStateImplementation(int state);

    protected:
        void customEvent(QEvent* event);
        void closeEvent(QCloseEvent* event);
        void pluginGuiQueryEvent(PluginGuiQueryEvent* event);

    private:
        void startEmulation();
        void setupActions();
        bool confirmMessage(const QString& msg);
        QList<QAction*> slotActions;
        QLabel* m_statusBarLabel;
        QActionGroup* m_uiActions;
        QPointer<QWidget> m_renderWindow;

    protected:
        bool eventFilter(QObject *obj, QEvent *ev);
};

#endif // __MAINWINDOW_H__

