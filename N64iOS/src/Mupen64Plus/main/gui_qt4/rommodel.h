/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rommodel.h                                              *
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

#ifndef __ROMMODEL_H__
#define __ROMMODEL_H__

#include <QAbstractItemModel>
#include <QStringList>
#include <QPixmap>
#include <QPair>
#include <QChar>

extern "C" {
    namespace core {
        #include "../romcache.h"
    }
}

class RomModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_ENUMS(Columns)
    Q_ENUMS(Role)
    public:
        enum Columns {
            Country = 0,
            GoodName,
            Status,
            UserComments,
            FileName,
            MD5Hash,
            InternalName,
            CRC1,
            CRC2,
            SaveType,
            Players,
            Size,
            CompressionType,
            ImageType,
            CICChip,
            Rumble,
            COLUMNS_SENTINEL, // keep this as the last entry
            LAST_VISIBLE_COLUMN = FileName // except this one may come after!
        };
        enum Role {
            Sort = Qt::UserRole,
            FullPath,
            ArchiveFile
        };

        RomModel(QObject* parent = 0);

        static RomModel* self();
        void update(unsigned int roms, unsigned short clear);

        // Model method implementations
        virtual QModelIndex index(int row, int column,
                           const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex& index,
                               int role = Qt::DisplayRole) const;
        virtual bool setData(const QModelIndex& index,
                             const QVariant& value,
                             int role = Qt::EditRole);
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                                     int role = Qt::DisplayRole) const;
        void settingsChanged();

    private:
        QPixmap countryFlag(QChar c) const;
        QString countryName(QChar c) const;

        QList<core::cache_entry*> m_romList;

        bool m_showFullPath;
        QStringList m_romDirectories;
        QMap<QChar, QPair<QString, QPixmap> > m_countryInfo;
};

#endif // __ROMMODEL_H__

