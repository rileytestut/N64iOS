/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - configdialog.c                                          *
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

/* configdialog.h - Handles the configuration dialog */

#ifndef __CONFIGDIALOG_H__
#define __CONFIGDIALOG_H__

#include <gtk/gtk.h>

typedef struct
{
    GtkWidget* dialog;

    GtkWidget* notebook;
    GtkWidget* configMupen;
    GtkWidget* configPlugins;
    GtkWidget* configRomBrowser;
    GtkWidget* configInputMappings;

    GtkWidget* coreInterpreterCheckButton;
    GtkWidget* coreDynaRecCheckButton;
    GtkWidget* corePureInterpCheckButton;
    GtkWidget* autoincSaveSlotCheckButton;
    GtkWidget* noaskCheckButton;
    GtkWidget* osdEnabled;
    GtkWidget* alwaysFullscreen;
    GList* toolbarStyleGList;
    GtkWidget* toolbarStyleCombo;
    GList* toolbarSizeGList;
    GtkWidget* toolbarSizeCombo;

    GtkWidget* graphicsCombo;
    GtkWidget* audioCombo;
    GtkWidget* inputCombo;
    GtkWidget* rspCombo;
    GList* graphicsPluginGList;
    GList* audioPluginGList;
    GList* inputPluginGList;
    GList* rspPluginGList;
    GtkWidget* romDirectoryList;
    GtkWidget* romDirsScanRecCheckButton;
    GtkWidget* romShowFullPathsCheckButton;
    GtkWidget* noCompiledJumpCheckButton;
    GtkWidget* noMemoryExpansion;

    GtkWidget* graphicsImage;
    GtkWidget* audioImage;
    GtkWidget* inputImage;
    GtkWidget* rspImage;

    GtkWidget* okButton;
} SConfigDialog;

extern SConfigDialog g_ConfigDialog;

void create_configDialog(void);
void show_configure();

#endif /* __CONFIGDIALOG_H__ */

