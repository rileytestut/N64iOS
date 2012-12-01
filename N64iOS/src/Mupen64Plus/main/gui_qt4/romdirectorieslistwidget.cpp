/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - romdirectorieslistwidget.cpp                            *
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

#include <QFileDialog>
#include "romdirectorieslistwidget.h"

RomDirectoriesListWidget::RomDirectoriesListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi(this);
    connect(addButton, SIGNAL(clicked()),
             this, SLOT(add()));
    connect(removeButton, SIGNAL(clicked()),
             this, SLOT(remove()));
    connect(removeAllButton, SIGNAL(clicked()),
             this, SLOT(removeAll()));
}

const QStringList& RomDirectoriesListWidget::directories() const
{
    return m_directories;
}

void RomDirectoriesListWidget::setDirectories(QStringList list)
{
    if (m_directories != list) {
        m_directories = list;
        updateListWidget();
        emit changed(m_directories);
    }
}

void RomDirectoriesListWidget::add()
{
    QString dir = QFileDialog::getExistingDirectory();
    if (!dir.isEmpty() && !m_directories.contains(dir)) {
        m_directories << dir;
        updateListWidget();
        emit changed(m_directories);
    }
}

void RomDirectoriesListWidget::remove()
{
    if (m_directories.count() > 0) {
        int index = romListWidget->currentRow();
        m_directories.removeAt(index);
        updateListWidget();
        emit changed(m_directories);
    }
}

void RomDirectoriesListWidget::removeAll()
{
    if (m_directories.count() > 0) {
        m_directories.clear();
        updateListWidget();
        emit changed(m_directories);
    }
}

void RomDirectoriesListWidget::updateListWidget()
{
    romListWidget->clear();
    romListWidget->addItems(m_directories);
}

