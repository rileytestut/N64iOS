/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - romdelegate.cpp                                         *
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

#include <QPainter>

#include "globals.h"
#include "rommodel.h"
#include "romdelegate.h"

static const int MAX_STATUS = 5;

RomDelegate::RomDelegate(QObject* parent)
: QItemDelegate(parent)
{}

void RomDelegate::paint(QPainter* painter,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) const
{
    switch (index.column()) {
        case RomModel::Status:
            {
                drawBackground(painter, option, index);
                int n = index.data(Qt::DisplayRole).toInt();
                const QRect& r = option.rect;

                drawStars(painter, r, n, MAX_STATUS);

                drawFocus(painter, option, option.rect);
            }
            break;
        default:
            QItemDelegate::paint(painter, option, index);
            break;
    }
}

QSize RomDelegate::sizeHint(const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    switch (index.column()) {
        case RomModel::Status:
            return QSize((16 + 2) * MAX_STATUS, 16 + 2);
            break;
        default:
            return QItemDelegate::sizeHint(option, index);
            break;
    }
}

