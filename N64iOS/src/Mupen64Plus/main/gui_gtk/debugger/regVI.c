/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regVI.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 DavFr                                              *
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

#include "regVI.h"

static GtkWidget *clRegVI;

// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32   gui_fantom_reg_VI[15];

static char *mnemonicVI[]=
{
    "VI_STATUS_REG",    "VI_DRAM_ADDR_REG",
    "VI_WIDTH_REG",     "VI_INTR_REG",
    "VI_CURRENT_LINE_REG",  "VI_TIMING_REG",
    "VI_V_SYNC_REG",    "VI_H_SYNC_REG",    "VI_H_SYNC_LEAP_REG",
    "VI_H_START_REG",   "VI_V_START_REG",
    "VI_V_BURST_REG",
    "VI_X_SCALE_REG",   "VI_Y_SCALE_REG",
    "OsTvType",
};

static unsigned int *regptrsVI[] = {
    &vi_register.vi_status,
    &vi_register.vi_origin,
    &vi_register.vi_width,
    &vi_register.vi_v_intr,
    &vi_register.vi_current,
    &vi_register.vi_burst,
    &vi_register.vi_v_sync,
    &vi_register.vi_h_sync,
    &vi_register.vi_leap,
    &vi_register.vi_h_start,
    &vi_register.vi_v_start,
    &vi_register.vi_v_burst,
    &vi_register.vi_x_scale,
    &vi_register.vi_y_scale,
    &rdram[0x300/4]
};

//]=-=-=-=-=-=-=[ Initialisation of Video Interface Display ]=-=-=-=-=-=-=-=-=[

void init_regVI()
{
    int i;

    frRegVI = gtk_frame_new("Video Interface");

    //=== Creation of Registers Value Display ==========/
    clRegVI = (GtkWidget *) init_hwreg_clist(15, mnemonicVI);
    gtk_container_add( GTK_CONTAINER(frRegVI), clRegVI);
    gtk_clist_set_selection_mode(GTK_CLIST(clRegVI), GTK_SELECTION_SINGLE);
    
    //=== Fantom Registers Initialisation ============/
    for( i=0; i<15; i++)
    {
        gui_fantom_reg_VI[i] = 0x12345678;
    }
}



//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Video Interface Display ]=-=-=-=-=-=-=-=--=[

void update_regVI()
{
    char txt[24];
    int i;

    gtk_clist_freeze(GTK_CLIST(clRegVI));

    for (i=0; i<15; i++) {
        if (gui_fantom_reg_VI[i] != (uint32)(*regptrsVI[i])) {
            gui_fantom_reg_VI[i] = (uint32)(*regptrsVI[i]);
            sprintf(txt, "%.8X", *regptrsVI[i]);
            gtk_clist_set_text(GTK_CLIST(clRegVI), i, 1, txt);
            gtk_clist_set_background(GTK_CLIST(clRegVI), i, &color_modif);
        } else {
            gtk_clist_set_background(GTK_CLIST(clRegVI), i, &color_ident);
        }
    }

    gtk_clist_thaw(GTK_CLIST(clRegVI));
}

