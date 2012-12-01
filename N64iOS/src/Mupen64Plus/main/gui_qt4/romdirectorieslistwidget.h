/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - romdirectorieslistwidget.h                              *
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

#ifndef __ROMDIRECTORIESLISTWIDGET_H__
#define __ROMDIRECTORIESLISTWIDGET_H__

/*
* This is a small wrapper class so we get a nice QStringList property that
* KConfigXT can handle automatically. This could be made into a general class.
*/

#include <QWidget>
#include <QStringList>
#include "ui_romdirectorieslistwidget.h"

class RomDirectoriesListWidget
    : public QWidget, private Ui::RomDirectoriesListWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList directories READ directories WRITE setDirectories
                USER true)
    public:
        RomDirectoriesListWidget(QWidget* parent = 0);
        const QStringList& directories() const;

    signals:
        void changed(const QStringList& list);

    public slots:
        void setDirectories(QStringList list);

    private slots:
        void add();
        void remove();
        void removeAll();

    private:
        void updateListWidget();
        QStringList m_directories;
};

#endif // __ROMDIRECTORIESLISTWIDGET_H__

