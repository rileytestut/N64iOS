/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - mainwidget.h                                            *
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

#ifndef __MAINWIDGET_H__
#define __MAINWIDGET_H__

#include <QWidget>
#include <QTimer>
#include <QUrl>

class QLabel;
class QVBoxLayout;
class QTreeView;
class QSortFilterProxyModel;
class QLineEdit;

#include "ui_mainwidget.h"

class MainWidget : public QWidget, public Ui_MainWidget
{
    Q_OBJECT
    public:
        MainWidget(QWidget* parent = 0);
        virtual ~MainWidget();
        QModelIndex getRomBrowserIndex();

    public slots:
        void showFilter(bool show);

    private slots:
        void resizeHeaderSections();
        void lineEditTextChanged();
        void filter();
        void treeViewDoubleClicked(const QModelIndex& index);
        void headerContextMenuRequested(const QPoint& pos);
        void hideHeaderSection(QAction* a);
        void treeViewContextMenuRequested(const QPoint& pos);

    signals:
        void itemCountChanged(int count);
        void romDoubleClicked(const QString& filename, unsigned int archivefile);

    protected:
        virtual bool eventFilter(QObject* obj, QEvent* event);

    private:
        QSortFilterProxyModel* m_proxyModel;
        QTimer m_timer;

        void load(const QModelIndex& index);
};

#endif // __MAINWIDGET_H__

