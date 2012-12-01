/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - romdelegate.h                                           *
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

#ifndef __ROM_DELEGATE_H__
#define __ROM_DELEGATE_H__

#include <QItemDelegate>
#include <QPixmap>

class RomDelegate : public QItemDelegate
{
    Q_OBJECT
    public:
        RomDelegate(QObject* parent = 0);

        virtual void paint(QPainter* painter,
                           const QStyleOptionViewItem& option,
                           const QModelIndex& index) const;
        virtual QSize sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& index) const;
};

#endif //__ROM_DELEGATE_H__

