/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - starlabel.cpp                                           *
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

#include <QPaintEvent>
#include <QPainter>

#include "starlabel.h"
#include "globals.h"

StarLabel::StarLabel(QWidget* parent)
: QLabel(parent), m_max(-1)
{
    setMinimumHeight(16);    
}

int StarLabel::max() const
{
    return m_max;
}

void StarLabel::setMax(int max)
{
    if (max != m_max) {
        m_max = max;
        update();
    }
}

void StarLabel::paintEvent(QPaintEvent* event)
{
    bool ok;
    int n = text().toInt(&ok);
    if (ok) {
        QPainter p(this);
        drawStars(&p, event->rect(), n, qMax(0, m_max));
    } else {
        QLabel::paintEvent(event);
    }
}

