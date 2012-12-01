/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - wainwidget.cpp                                          *
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

#include <QVBoxLayout>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QHeaderView>
#include <QKeyEvent>
#include <QApplication>
#include <Qt>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QMenu>
#include <QActionGroup>
#include <QDialog>
#include <QPushButton>

#include "mainwidget.h"
#include "rommodel.h"
#include "romdelegate.h"
#include "globals.h"
#include "rominfodialog.h"

namespace core {
    extern "C" {
        #include "../romcache.h"
    }
}

MainWidget::MainWidget(QWidget* parent)
    : QWidget(parent)
    , m_proxyModel(0)
{
    setupUi(this);

    lineEdit->installEventFilter(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(RomModel::self());
    m_proxyModel->setFilterKeyColumn(-1); // search all columns
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortRole(RomModel::Sort);

    treeView->setModel(m_proxyModel);
    treeView->sortByColumn(RomModel::GoodName, Qt::AscendingOrder);
    treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    treeView->setFocusProxy(lineEdit);
    treeView->setItemDelegate(new RomDelegate(this));

    connect(treeView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(treeViewContextMenuRequested(QPoint)));

    QSettings s;
    QByteArray headerState = s.value("RomBrowserHeadersState").toByteArray();
    if (!headerState.isEmpty()) {
        treeView->header()->restoreState(headerState);
    } else {
        for (int i = 0; i < treeView->header()->count(); i++) {
            treeView->header()->hideSection(i);
        }
        for (int i = 0; i <= RomModel::LAST_VISIBLE_COLUMN; i++) {
            treeView->header()->showSection(i);
        }
        resizeHeaderSections();
    }

    treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView->header(), SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(headerContextMenuRequested(QPoint)));

    m_timer.setSingleShot(true);

    connect(lineEdit, SIGNAL(textChanged(QString)),
             this, SLOT(lineEditTextChanged()));
    connect(&m_timer, SIGNAL(timeout()),
             this, SLOT(filter()));
    connect(m_proxyModel, SIGNAL(modelReset()),
             this, SLOT(resizeHeaderSections()));
    connect(m_proxyModel, SIGNAL(modelReset()),
             this, SLOT(filter()));
    connect(m_proxyModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
             this, SLOT(resizeHeaderSections()));
    connect(m_proxyModel, SIGNAL(layoutChanged()),
             this, SLOT(resizeHeaderSections()));
    connect(treeView, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(treeViewDoubleClicked(QModelIndex)));

    //lineEdit->setFocus();
    QTimer::singleShot(0, this, SLOT(filter())); // so we emit the base item count
}

MainWidget::~MainWidget()
{
    QSettings s;
    QByteArray headerState = treeView->header()->saveState();
    s.setValue("RomBrowserHeadersState", headerState);
}

QModelIndex MainWidget::getRomBrowserIndex()
{
    return treeView->currentIndex();
}

void MainWidget::showFilter(bool show)
{
    label->setVisible(show);
    lineEdit->setVisible(show);
    lineEdit->clear();
}

void MainWidget::resizeHeaderSections()
{
    QString filter = lineEdit->text();
    m_proxyModel->setFilterFixedString("");
    treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    m_proxyModel->setFilterFixedString(filter);
    emit itemCountChanged(m_proxyModel->rowCount());
}

void MainWidget::lineEditTextChanged()
{
    if (m_timer.isActive()) {
        m_timer.stop();
    }
    m_timer.start(50);
}

void MainWidget::filter()
{
    m_proxyModel->setFilterFixedString(lineEdit->text());
    emit itemCountChanged(m_proxyModel->rowCount());
}

void MainWidget::treeViewDoubleClicked(const QModelIndex& index)
{
    load(index);
}

void MainWidget::headerContextMenuRequested(const QPoint& pos)
{
    QHeaderView* header = treeView->header();
    QAbstractItemModel* model = header->model();
    Qt::Orientation o = header->orientation();

    QMenu menu;
    QActionGroup group(this);

    group.setExclusive(false);
    connect(&group, SIGNAL(triggered(QAction*)),
            this, SLOT(hideHeaderSection(QAction*)));

    for (int i = 0; i < header->count(); i++) {
        QString title = model->headerData(i, o, Qt::DisplayRole).toString();
        QAction* a = menu.addAction(title);
        a->setCheckable(true);
        a->setChecked(!header->isSectionHidden(i));
        a->setData(i);
        group.addAction(a);
    }

    menu.exec(header->mapToGlobal(pos));
}

void MainWidget::hideHeaderSection(QAction* a)
{
    int section = a->data().toInt();
    treeView->header()->setSectionHidden(section, !a->isChecked());
    resizeHeaderSections();
}

void MainWidget::treeViewContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QMenu m;
    QAction* loadAction = m.addAction(tr("Load"));
    loadAction->setIcon(icon("media-playback-start.png"));
    QAction* propertiesAction = m.addAction(tr("Properties..."));
    propertiesAction->setIcon(icon("document-properties.png"));
    m.addSeparator();
    QAction* refreshAction = m.addAction(tr("Refresh Rom List"));
    refreshAction->setIcon(icon("view-refresh.png"));

    QAction* a = m.exec(QCursor::pos());
    if (a == loadAction) {
        load(index);
    } else if (a == propertiesAction) {
        int row = index.row();
        RomInfoDialog* d = new RomInfoDialog(this);
        d->statusLabel->setMax(5);
        d->flagLabel->setPixmap(index.sibling(row, RomModel::Country).data(Qt::DecorationRole).value<QPixmap>());
        d->countryLabel->setText(index.sibling(row, RomModel::Country).data().toString());
        d->goodNameLabel->setText(index.sibling(row, RomModel::GoodName).data().toString());
        d->statusLabel->setText(index.sibling(row, RomModel::Status).data().toString());
        d->lineEdit->setText(index.sibling(row, RomModel::UserComments).data().toString());
        d->fileNameLabel->setText(index.sibling(row, RomModel::FileName).data().toString());
        d->md5HashLabel->setText(index.sibling(row, RomModel::MD5Hash).data().toString());
        d->internalNameLabel->setText(index.sibling(row, RomModel::InternalName).data().toString());
        d->crc1Label->setText(index.sibling(row, RomModel::CRC1).data().toString());
        d->crc2Label->setText(index.sibling(row, RomModel::CRC2).data().toString());
        d->saveTypeLabel->setText(index.sibling(row, RomModel::SaveType).data().toString());
        d->playersLabel->setText(index.sibling(row, RomModel::Players).data().toString());
        d->sizeLabel->setText(index.sibling(row, RomModel::Size).data().toString());
        d->compressionLabel->setText(index.sibling(row, RomModel::CompressionType).data().toString());
        d->imageTypeLabel->setText(index.sibling(row, RomModel::ImageType).data().toString());
        d->rumbleLabel->setText(index.sibling(row, RomModel::Rumble).data().toString());
        d->cicChipLabel->setText(index.sibling(row, RomModel::CICChip).data().toString());
        d->rumbleLabel->setText(index.sibling(row, RomModel::Rumble).data().toString());
        d->fullPathLabel->setText(index.data(RomModel::FullPath).toString());
        d->index = m_proxyModel->mapToSource(index);
        d->show();
    } else if (a == refreshAction) {
        core::g_romcache.rcstask = core::RCS_RESCAN;
    }
}

bool MainWidget::eventFilter(QObject* obj, QEvent* event)
{
    bool filtered = false;

    if (obj == lineEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            switch(keyEvent->key()) {
                case Qt::Key_Up:
                case Qt::Key_Down:
                case Qt::Key_PageUp:
                case Qt::Key_PageDown:
                    QApplication::sendEvent(treeView, keyEvent);
                    filtered = true;
                    break;
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    load(treeView->currentIndex());
                    filtered = true;
                    break;
            }
        }
    }

    return filtered;
}

void MainWidget::load(const QModelIndex& index)
{
    QString filename = index.data(RomModel::FullPath).toString();
    unsigned int archivefile = index.data(RomModel::ArchiveFile).toUInt();
    if (!filename.isEmpty()) {
        emit romDoubleClicked(filename, archivefile);
    }
}

