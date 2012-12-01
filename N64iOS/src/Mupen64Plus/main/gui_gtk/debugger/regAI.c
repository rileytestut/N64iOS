/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regAI.c                                                 *
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

#include "regAI.h"

static GtkWidget *clRegAI;

// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32   gui_fantom_reg_AI[6];

static char *mnemonicAI[]=
{
    "AI_DRAM_ADDR_REG", "AI_LEN_REG",
    "AI_CONTROL_REG",   "AI_STATUS_REG",
    "AI_DACRATE_REG",   "AI_BITRATE_REG",
};

static unsigned int *regptrsAI[] = {
    &ai_register.ai_dram_addr, 
    &ai_register.ai_len,
    &ai_register.ai_control,
    &ai_register.ai_status,
    &ai_register.ai_dacrate,
    &ai_register.ai_bitrate
};

//]=-=-=-=-=-=-=-=[ Initialisation of Audio Interface Display ]=-=-=-=-=-=-=-=[

void init_regAI()
{
    int i;

    frRegAI = gtk_frame_new("Audio Interface");

    //=== Creation of Registers Value Display ==========/
    clRegAI = (GtkWidget *) init_hwreg_clist(6, mnemonicAI);
    gtk_container_add( GTK_CONTAINER(frRegAI), clRegAI);
    gtk_clist_set_selection_mode(GTK_CLIST(clRegAI), GTK_SELECTION_SINGLE);

    //=== Signal Connection ==========================/
//TODO: I plan to enable user to modify cop0 registers 'on-the-fly'.
//  gtk_signal_connect( GTK_OBJECT(clRegCop0), "button_press_event",
//              GTK_SIGNAL_FUNC(on_click), clRegCop0 );

    //=== Fantom Registers Initialisation ============/
    for( i=0; i<6; i++)
    {
        gui_fantom_reg_AI[i] = 0x12345678;
    }
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop0 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_regAI()
{
    char txt[24];
    int i;
    
    gtk_clist_freeze(GTK_CLIST(clRegAI));

    for (i=0; i<6; i++) {
        if (gui_fantom_reg_AI[i] != (uint32)(*regptrsAI[i])) {
            gui_fantom_reg_AI[i] = (uint32)(*regptrsAI[i]);
            sprintf(txt, "%.8X", *regptrsAI[i]);
            gtk_clist_set_text(GTK_CLIST(clRegAI), i, 1, txt );
            gtk_clist_set_background(GTK_CLIST(clRegAI), i, &color_modif);
        } else {
            gtk_clist_set_background( GTK_CLIST(clRegAI), i, &color_ident);
        }
    }

    gtk_clist_thaw(GTK_CLIST(clRegAI));
}

