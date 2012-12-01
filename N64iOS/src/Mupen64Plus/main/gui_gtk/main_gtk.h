/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main_gtk.h                                              *
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

/* main_gtk.c - Handles the main window and 'glues' it with other windows */

#ifndef __MAIN_GTK_H__
#define __MAIN_GTK_H__

#include "main_gtk.h"

#include "../gui.h"

typedef struct
{
    GtkWidget* window;
    GtkWidget* toplevelVBox;  /* Topevel vbox for main window. */

    /* Pointers to GUI widgets whose state can be changed while running, 
    such as toggling visibile / enabled, or chaning icons. */
    GtkWidget* dialogErrorImage;
    GtkWidget* dialogQuestionImage;

    GtkWidget* menuBar;
    GtkWidget* openMenuImage;

    GtkWidget* openRomMenuImage;
    GtkWidget* closeRomMenuImage;
    GtkWidget* quitMenuImage;

    GtkWidget* playMenuImage;
    GtkWidget* pauseMenuItem;
    GtkWidget* pauseMenuImage;
    GtkWidget* stopMenuItem;
    GtkWidget* stopMenuImage;
    GtkWidget* saveStateMenuItem;
    GtkWidget* saveStateMenuImage;
    GtkWidget* saveStateAsMenuItem;
    GtkWidget* saveStateAsMenuImage;
    GtkWidget* loadStateMenuItem;
    GtkWidget* loadStateMenuImage;
    GtkWidget* loadStateFromMenuItem;
    GtkWidget* loadStateFromMenuImage;

    GtkWidget* configureMenuImage;
    GtkWidget* graphicsMenuImage;
    GtkWidget* audioMenuImage;
    GtkWidget* inputMenuImage;
    GtkWidget* rspMenuImage;
    GtkWidget* fullscreenMenuItem;
    GtkWidget* fullscreenMenuImage;

    GtkWidget* aboutMenuImage;

    GtkWidget* toolBar;
    GtkWidget* openButtonImage;
    GtkWidget* playButtonItem;
    GtkWidget* playButtonImage;
    GtkWidget* pauseButtonItem;
    GtkWidget* pauseButtonImage;
    GtkWidget* stopButtonItem;
    GtkWidget* stopButtonImage;
    GtkWidget* saveStateButtonItem;
    GtkWidget* saveStateButtonImage;
    GtkWidget* loadStateButtonItem;
    GtkWidget* loadStateButtonImage;
    GtkWidget* fullscreenButtonItem;
    GtkWidget* fullscreenButtonImage;
    GtkWidget* configureButtonImage;

    GtkWidget* filterBar; /* Container object for filter, toggle visability. */
    GtkWidget* filter; /* Entry for filter text. */
    GtkAccelGroup* accelGroup;
    GtkAccelGroup* accelUnsafe; /* GtkAccelGroup for keys without Metas. Prevents GtkEntry widgets. */
    gboolean accelUnsafeActive; /* From getting keypresses, so must be deactivated. */

    GtkWidget* romScrolledWindow;
    /* Make two TreeViews, a visable manually filtered one for the Display, and a
    Non-visable FullList from which we can filter. */
    GtkWidget* romDisplay;
    GtkWidget* romFullList;
    GtkTreeViewColumn* column[17]; /* Column pointers in rombrowser. */
    char column_names[16][2][128]; /* Localized names and English config system strings. */
    int romSortColumn;
    GtkSortType romSortType;
    GtkWidget* romHeaderMenu; /* Context menu for rombrowser to control visible columns. */
    GtkWidget* playRomItem;
    GtkWidget* romPropertiesItem;

    GtkWidget* playRombrowserImage;
    GtkWidget* propertiesRombrowserImage;
    GtkWidget* refreshRombrowserImage;

    GtkWidget* statusBar; /* Statusbar message stack. */
    GtkWidget* statusBarHBox; /* Container object for statusbar, toggle visability. */
} SMainWindow;

extern SMainWindow g_MainWindow;
extern GdkPixbuf *australia, *europe, *france, *germany, *italy, *japan, *spain, *usa, *japanusa, *n64cart, *star, *staroff;

/* View defines */
#define TOOLBAR 1
#define FILTER 2
#define STATUSBAR 3

#endif /* __MAIN_GTK_H__ */

