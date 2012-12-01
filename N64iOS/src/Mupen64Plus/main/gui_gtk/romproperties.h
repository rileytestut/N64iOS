/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - romproperties.h                                         *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Blight                                             *
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

/* romproperties.h - Handles rom properties dialog */

#ifndef __ROMPROPERTIES_H__
#define __ROMPROPERTIES_H__

#include "main_gtk.h"

#include "../romcache.h"

typedef struct
{
    GtkWidget* dialog;
    GtkWidget* filenameEntry;
    GtkWidget* goodnameEntry;
    GtkWidget* flag;
    GtkWidget* countryEntry;
    GtkWidget* status[5];
    GtkWidget* fullpathEntry;
    GtkWidget* crc1Entry;
    GtkWidget* crc2Entry;
    GtkWidget* md5Entry;
    GtkWidget* internalnameEntry;
    GtkWidget* sizeEntry;
    GtkWidget* savetypeEntry;
    GtkWidget* playersEntry;
    GtkWidget* compressiontypeEntry;
    GtkWidget* imagetypeEntry;
    GtkWidget* cicchipEntry;
    GtkWidget* rumbleEntry;
    GtkWidget* commentsEntry;
    GtkWidget* okButton;
    GtkTreeIter iter;
    cache_entry* entry;
} SRomPropertiesDialog;

extern SRomPropertiesDialog g_RomPropDialog;

void create_rom_properties();
void show_rom_properties();

#endif /* __ROMPROPERTIES_H__ */

