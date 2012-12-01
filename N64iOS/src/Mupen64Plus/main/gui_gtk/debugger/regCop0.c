/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regCop0.c                                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 davFr                                              *
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

#include "regCop0.h"

static GtkWidget *clRegCop0;

// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static sint64   gui_fantom_reg_cop0[32];

static char *mnemonicCop0[]=
{
    "Index",    "Random",   "EntryLo0", "EntryLo1",
    "Context",  "PageMask", "Wired",    "----",
    "BadVAddr", "Count",    "EntryHi",  "Compare",
    "SR",       "Cause",    "EPC",      "PRid",
    "Config",   "LLAddr",   "WatchLo",  "WatchHi",
    "Xcontext", "----",     "----",     "----",
    "----",     "----",     "PErr",     "CacheErr",
    "TagLo",    "TagHi",    "ErrorEPC", "----",
};


//]=-=-=-=-=-=-=-=[ Initialisation of Cop0 Registre Display ]=-=-=-=-=-=-=-=-=[

void init_regCop0()
{
    int i;
    char *txt[3];
    char txt_regnum[6];

    regCop0_opened = 1;
    frCop0 = gtk_frame_new("cop0");

    //=== Creation of Registers Value Display ==========/
    clRegCop0 = gtk_clist_new(3);
    for (i=0; i<3; i++) {
        gtk_clist_set_column_auto_resize(GTK_CLIST(clRegCop0), i, TRUE);
    }
    gtk_widget_modify_font(clRegCop0, debugger_font_desc);
    gtk_container_add( GTK_CONTAINER(frCop0), clRegCop0 );
    gtk_clist_set_selection_mode( GTK_CLIST(clRegCop0), GTK_SELECTION_SINGLE);
    txt[0] = txt_regnum;
    txt[1] = "MMMMMMMM";
    for( i=0; i<32; i++) {
        sprintf( txt[0], "%d", i);
        txt[2] = mnemonicCop0[i];
        gtk_clist_append( GTK_CLIST(clRegCop0), txt);
    }

    //=== Signal Connection ==========================/
//TODO: I plan to enable user to modify cop0 registers 'on-the-fly'.

//  gtk_signal_connect( GTK_OBJECT(clRegCop0), "button_press_event",
//              GTK_SIGNAL_FUNC(on_click), clRegCop0 );

    //=== Fantom Registers Initialisation ============/
    for( i=0; i<32; i++)
    {
        gui_fantom_reg_cop0[i] = 0x1234567890LL;
        //Should be put to the least probable value.
    }
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop0 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_regCop0()
{
    int i;
    char txt[24];

    gtk_clist_freeze( GTK_CLIST(clRegCop0) );

    for(i=0; i<32; i++)
    {
        if( gui_fantom_reg_cop0[i] != reg_cop0[i] )
        {
            gui_fantom_reg_cop0[i] = reg_cop0[i];
            sprintf( txt, "%.8llX", (sint64)reg_cop0[i] );
            gtk_clist_set_text( GTK_CLIST(clRegCop0), i, 1, txt );
            gtk_clist_set_background( GTK_CLIST(clRegCop0), i, &color_modif);
        }
        else
        {
            gtk_clist_set_background( GTK_CLIST(clRegCop0), i, &color_ident);
        }
    }

    gtk_clist_thaw( GTK_CLIST(clRegCop0) );
}

