/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regPI.c                                                 *
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

#include "regPI.h"

static GtkWidget *clRegPI;

// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32   gui_fantom_reg_PI[13];

static char *mnemonicPI[]=
{
    "PI_DRAM_ADDR_REG", "PI_CART_ADDR_REG",
    "PI_RD_LEN_REG",    "PI_WR_LEN_REG",
    "PI_STATUS_REG",
    "PI_BSD_DOM1_LAT_REG",  "PI_BSD_DOM1_PWD_REG",
    "PI_BSD_DOM1_PGS_REG",  "PI_BSD_DOM1_RLS_REG",
    "PI_BSD_DOM2_LAT_REG",  "PI_BSD_DOM2_PWD_REG",
    "PI_BSD_DOM2_PGS_REG",  "PI_BSD_DOM2_RLS_REG",
};

static unsigned int *regptrsPI[] = {
    &pi_register.pi_dram_addr_reg,
    &pi_register.pi_cart_addr_reg,
    &pi_register.pi_rd_len_reg,
    &pi_register.pi_wr_len_reg,
    &pi_register.read_pi_status_reg,
    &pi_register.pi_bsd_dom1_lat_reg,
    &pi_register.pi_bsd_dom1_pwd_reg,
    &pi_register.pi_bsd_dom1_pgs_reg,
    &pi_register.pi_bsd_dom1_rls_reg,
    &pi_register.pi_bsd_dom2_lat_reg,
    &pi_register.pi_bsd_dom2_pwd_reg,
    &pi_register.pi_bsd_dom2_pgs_reg,
    &pi_register.pi_bsd_dom2_rls_reg
};


//]=-=-=-=-=-=-=-=[ Initialisation of Peripheral Interface Display ]=-=-=-=-=-[

void init_regPI()
{
    int i;

    frRegPI = gtk_frame_new("Video Interface");

    //=== Creation of Registers Value Display =========/
    clRegPI = (GtkWidget *) init_hwreg_clist(13, mnemonicPI);
    gtk_container_add( GTK_CONTAINER(frRegPI), clRegPI);
    gtk_clist_set_selection_mode(GTK_CLIST(clRegPI), GTK_SELECTION_SINGLE);

    //=== Fantom Registers Initialisation =============/
    for( i=0; i<13; i++)
    {
        gui_fantom_reg_PI[i] = 0x12345678;
    }
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Peripheral Interface Display ]=-=-=-=-=-=-=[

void update_regPI()
{
    char txt[24];
    int i;

    gtk_clist_freeze(GTK_CLIST(clRegPI));
    
    for (i=0; i<13; i++) {
        if (gui_fantom_reg_PI[i] != (uint32)(*regptrsPI[i])) {
            gui_fantom_reg_PI[i] = (uint32)(*regptrsPI[i]);
            sprintf(txt, "%.8X", *regptrsPI[i]);
            gtk_clist_set_text(GTK_CLIST(clRegPI), i, 1, txt);
            gtk_clist_set_background(GTK_CLIST(clRegPI), i, &color_modif);
        } else {
            gtk_clist_set_background(GTK_CLIST(clRegPI), i, &color_ident);
        }
    }

    gtk_clist_thaw(GTK_CLIST(clRegPI));
}

