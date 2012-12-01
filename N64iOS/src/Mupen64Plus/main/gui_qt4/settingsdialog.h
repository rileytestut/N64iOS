/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - settingsdialog.h                                        *
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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include <QDialog>
#include "ui_settingsdialog.h"

class QAbstractButton;

class SettingsDialog : public QDialog, public Ui_SettingsDialog
{
    Q_OBJECT
    public:
        SettingsDialog(QWidget* parent = 0);

    public slots:
        void accept();

    private slots:
        void on_aboutAudioPluginButton_clicked();
        void on_configAudioPluginButton_clicked();
        void on_testAudioPluginButton_clicked();
        void on_aboutInputPluginButton_clicked();
        void on_configInputPluginButton_clicked();
        void on_testInputPluginButton_clicked();
        void on_aboutRspPluginButton_clicked();
        void on_configRspPluginButton_clicked();
        void on_testRspPluginButton_clicked();
        void on_aboutGraphicsPluginButton_clicked();
        void on_configGraphicsPluginButton_clicked();
        void on_testGraphicsPluginButton_clicked();

        void buttonClicked(QAbstractButton* button);
        void pageChanged(int);

    private:
        void readSettings();
        void writeSettings();
};

#endif // __SETTINGSDIALOG_H__

