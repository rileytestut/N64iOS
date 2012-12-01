/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - aboutdialog.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
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

/*  aboutdialog.c - Handles the about box */

#include "aboutdialog.h"
#include "main_gtk.h"

#include "../main.h"
#include "../version.h"
#include "../translate.h"

#include <gtk/gtk.h>

void callback_about_mupen64plus(GtkWidget* widget, gpointer data)
{
    const gchar* authors[] =
        {
        "Richard Goedeken (Richard42)",
        "John Chadwick (NMN)",
        "James Hood (Ebenblues)",
        "Scott Gorman (okaygo)",
        "Scott Knauert (Tillin9)",
        "Jesse Dean (DarkJezter)",
        "Louai Al-Khanji (slougi)",
        "Bob Forder (orbitaldecay)",
        "Jason Espinosa (hasone)",
        "Dylan Wagstaff (Pyromanik)",
        "HyperHacker",
        "Hacktarux",
        "Dave2001",
        "Zilmar",
        "Gregor Anich (Blight)",
        "Juha Luotio (JttL)",
        "and many more...",
        NULL
        };

    const gchar* license =
        "This program is free software; you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation; either version 2 of the License, or\n"
        "(at your option) any later version.\n"
         "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the\n"
        "Free Software Foundation, Inc.,\n"
        "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n";

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(get_iconpath("mupen64plus-large.png"), NULL);

    gtk_show_about_dialog(GTK_WINDOW(g_MainWindow.window), "version", PLUGIN_VERSION, "copyright", "(C) 2007-2008 The Mupen64Plus Team", "license", license, "website", "http://code.google.com/p/mupen64plus/", "comments", "Mupen64plus is an open source Nintendo 64 emulator.", "authors", authors, "logo", pixbuf, "title", "About Mupen64plus", NULL);
}

