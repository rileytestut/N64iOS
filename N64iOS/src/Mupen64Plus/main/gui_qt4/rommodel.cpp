/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rommodel.cpp                                            *
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

#include <QStringList>
#include <QDir>
#include <QFont>
#include <QUrl>
#include <QMetaEnum>
#include <QDateTime>
#include <QWidget>

#include <cstdio>

#include "rommodel.h"
#include "globals.h"

namespace core {
    extern "C" {
        #include "../rom.h"
        #include "../romcache.h"
        #include "../main.h"
        #include "../config.h"
        #include "../util.h"
    }
}

RomModel::RomModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_showFullPath(core::config_get_bool("RomBrowserShowFullPaths", FALSE))
    , m_romDirectories(romDirectories())
{
    QString file;
    char* iconpath = core::get_iconspath();
    QPixmap paustralia(file.sprintf("%s%s", iconpath, "australia.png"));
    QPixmap peurope(file.sprintf("%s%s", iconpath, "europe.png"));
    QPixmap pfrance(file.sprintf("%s%s", iconpath, "france.png"));
    QPixmap pgermany(file.sprintf("%s%s", iconpath, "germany.png"));
    QPixmap pitaly(file.sprintf("%s%s", iconpath, "italy.png"));
    QPixmap pjapan(file.sprintf("%s%s", iconpath, "japan.png"));
    QPixmap pspain(file.sprintf("%s%s", iconpath, "spain.png"));
    QPixmap pusa(file.sprintf("%s%s", iconpath, "usa.png"));
    QPixmap pjapanusa(file.sprintf("%s%s", iconpath, "japanusa.png"));
    QPixmap pn64cart(file.sprintf("%s%s", iconpath, "mupen64cart.png"));

    QPair<QString, QPixmap> demo(tr("Demo"), pn64cart);
    QPair<QString, QPixmap> beta(tr("Beta"), pn64cart);
    QPair<QString, QPixmap> japanusa(tr("Japan/USA"), pjapanusa);
    QPair<QString, QPixmap> usa(tr("USA"), pusa);
    QPair<QString, QPixmap> germany(tr("Germany"), pgermany);
    QPair<QString, QPixmap> france(tr("France"), pfrance);
    QPair<QString, QPixmap> italy(tr("Italy"), pitaly);
    QPair<QString, QPixmap> japan(tr("Japan"), pjapan);
    QPair<QString, QPixmap> spain(tr("Spain"), pspain);
    QPair<QString, QPixmap> australia(tr("Australia"), paustralia);
    QPair<QString, QPixmap> europe(tr("Europe"), peurope);
    QPair<QString, QPixmap> unknown(tr("Unknown"), pn64cart);

    m_countryInfo[char(0)] = demo;
    m_countryInfo['7'] = beta;
    m_countryInfo[char(0x41)] = japanusa;
    m_countryInfo[char(0x44)] = germany;
    m_countryInfo[char(0x45)] = usa;
    m_countryInfo[char(0x46)] = france;
    m_countryInfo['I'] = italy;
    m_countryInfo[char(0x4A)] = japan;
    m_countryInfo['S'] = spain;
    m_countryInfo[char(0x55)] = australia;
    m_countryInfo[char(0x59)] = australia;
    m_countryInfo[char(0x50)] = europe;
    m_countryInfo[char(0x58)] = europe;
    m_countryInfo[char(0x20)] = europe;
    m_countryInfo[char(0x21)] = europe;
    m_countryInfo[char(0x38)] = europe;
    m_countryInfo[char(0x70)] = europe;
    m_countryInfo['?'] = unknown;
}

RomModel* RomModel::self()
{
    static RomModel* instance = new RomModel;
    return instance;
}

void RomModel::update(unsigned int roms, unsigned short clear)
{
    //If clear flag is set, clear the GUI rombrowser.
    if (clear) {
        m_romList.clear();
        reset();
    }

    unsigned int arrayroms = static_cast<unsigned int>(m_romList.count());

    //If there are currently more ROMs in cache than GUI rombrowser, add them.
    if (roms > arrayroms) {
        core::cache_entry *entry;
        entry = core::g_romcache.top;

        //Advance cache pointer.
        for (unsigned i = 0; i < arrayroms; i++) {
            entry = entry->next;
            if (!entry) {
                return;
            }
        }

        for (unsigned i = 0; (i < roms) && (entry != NULL); i++) {
            //Actually add entries to RomModel
            m_romList << entry;
            //printf("Added: %s\n", entry->inientry->goodname);
            entry = entry->next;
        }
        reset();
    }
}

QModelIndex RomModel::index(int row, int column,
                             const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column, m_romList.at(row));
}
 
QModelIndex RomModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int RomModel::rowCount(const QModelIndex& parent) const
{
    int retval = 0;

    if (!parent.isValid()) {
        retval = m_romList.count();
    }

    return retval;
}

int RomModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 16;
}

QVariant RomModel::data(const QModelIndex& index, int role) const
{
    QVariant data;

    if (index.isValid() && (index.row() < m_romList.size())) {
        const core::cache_entry* entry = m_romList[index.row()];

        if (role == Qt::DisplayRole || role == Sort) {
            char* buffer;
            switch(index.column()) {
                case Country:
                    data = countryName(entry->countrycode);
                    break;
                case GoodName:
                    data = QString(entry->inientry->goodname);
                    break;
                case Status:
                    data = entry->inientry->status;
                    break;
                case UserComments:
                    data = QString(entry->usercomments);
                    break;
                case FileName:
                    if (core::config_get_bool("RomBrowserShowFullPaths", FALSE)) {
                        data = entry->filename;
                    } else {
                        data = QFileInfo(entry->filename).fileName();
                    }
                    break;
                case MD5Hash:
                    int counter;
                    buffer = (char*)calloc(33,sizeof(char));
                    for ( counter = 0; counter < 16; ++counter )
                        std::sprintf(buffer+counter*2, "%02X", entry->md5[counter]);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case InternalName:
                    data = QString(entry->internalname);
                    break;
                case CRC1:
                    data = QString().sprintf("%08X", entry->crc1);
                    break;
                case CRC2:
                    data = QString().sprintf("%08X", entry->crc2);
                    break;
                case SaveType:
                    buffer = (char*)calloc(16,sizeof(char));
                    core::savestring(entry->inientry->savetype, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case Players:
                    buffer = (char*)calloc(16,sizeof(char));
                    core::playersstring(entry->inientry->players, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case Size:
                    data = tr("%0 Mbit").arg((entry->romsize*8) / 1024 / 1024);
                    break;
                case CompressionType:
                    buffer = (char*)calloc(16,sizeof(char));
                    core::compressionstring(entry->compressiontype, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case ImageType:
                    buffer = (char*)calloc(32,sizeof(char));
                    core::imagestring(entry->imagetype, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case CICChip:
                    buffer = (char*)calloc(16,sizeof(char));
                    core::cicstring(entry->cic, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                case Rumble:
                    buffer = (char*)calloc(16,sizeof(char));
                    core::rumblestring(entry->inientry->rumble, buffer);
                    data = QString(buffer);
                    free(buffer);
                    break;
                default:
                    data = tr("Internal error");
                    break;
            }
        } else if (role == Qt::FontRole) {
            switch(index.column()) {
                case Size:
                case InternalName:
                case MD5Hash:
                case CRC1:
                case CRC2:
                    data = QFont("monospace");
                    break;
            }
        } else if (role == Qt::TextAlignmentRole) {
            switch(index.column()) {
                case Size:
                case MD5Hash:
                case CRC1:
                case CRC2:
                    data = Qt::AlignRight;
                    break;
            }
        } else if (role == Qt::DecorationRole) {
            switch(index.column()) {
                case Country:
                    data = countryFlag(entry->countrycode);
                    break;
                }
        //Assign Role enums here to retrive information.
        } else if (role == FullPath) {
            data = entry->filename;
        } else if (role == ArchiveFile) {
            data = entry->archivefile;
        }
    }

    return data;
}

bool RomModel::setData(const QModelIndex& index, const QVariant& value,
                        int role)
{
    bool result = false;
    if (index.isValid() &&
        index.row() >= 0 &&
        index.row() < m_romList.count() &&
        role == Qt::EditRole) {
        switch (index.column()) {
            case UserComments:
                result = true;
                qstrncpy(m_romList.at(index.row())->usercomments,
                         qPrintable(value.toString()),
                         COMMENT_MAXLENGTH);
                core::g_romcache.rcstask = core::RCS_WRITE_CACHE;
                emit dataChanged(index, index);
                break;
        }
    }
    return result;
}

QVariant RomModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    QVariant data;
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
        case Country: return tr("Country");
        case GoodName: return tr("Good Name");
        case Status: return tr("Status");
        case UserComments: return tr("User Comments");
        case FileName: return tr("File Name");
        case MD5Hash: return tr("MD5 Hash");
        case InternalName: return tr("Internal Name");
        case CRC1: return tr("CRC1");
        case CRC2: return tr("CRC2");
        case SaveType: return tr("Save Type");
        case Players: return tr("Players");
        case Size: return tr("Size");
        case CompressionType: return tr("Compression Type");
        case ImageType: return tr("Image Type");
        case CICChip: return tr("CIC Chip");
        case Rumble: return tr("Rumble");
        default: return QVariant();
    }
}

void RomModel::settingsChanged()
{
    if (romDirectories() != m_romDirectories) {
        m_romDirectories = romDirectories();
        core::g_romcache.rcstask = core::RCS_RESCAN;
    }
    if (m_showFullPath != core::config_get_bool("RomBrowserShowFullPaths", FALSE)) {
        m_showFullPath = core::config_get_bool("RomBrowserShowFullPaths", FALSE);
        emit dataChanged(
            index(0, FileName),
            index(columnCount() - 1, FileName)
        );
    }
}

QPixmap RomModel::countryFlag(QChar c) const
{
    if (!m_countryInfo.contains(c)) {
        c = '?';
    }
    return m_countryInfo[c].second;
}

QString RomModel::countryName(QChar c) const
{
    if (!m_countryInfo.contains(c)) {
        c = '?';
    }
    return m_countryInfo[c].first;
}

