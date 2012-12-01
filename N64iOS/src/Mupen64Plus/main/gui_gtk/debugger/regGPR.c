/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regGPR.c                                                *
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

#include "regGPR.h"

// On garde une copie des valeurs affichees a l'ecran, pour accelerer la maj de
// l'affichage.
// Les "registres fantomes" evitent de raffraichir l'affichage de chaque
// registre. Seules les modifications sont affichees a l'ecran.
static sint64   gui_fantom_gpr[32];

static char *mnemonicGPR[]= {
    "R0",   "AT",   "V0",   "V1",
    "A0",   "A1",   "A2",   "A3",
    "T0",   "T1",   "T2",   "T3",
    "T4",   "T5",   "T6",   "T7",
    "S0",   "S1",   "S2",   "S3",
    "S4",   "S5",   "S6",   "S7",
    "T8",   "T9",   "K0",   "K1",
    "GP",   "SP",   "S8",   "RA",
};


static GtkWidget *clGPR;
//static GtkWidget *edGPR[32];


//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void init_GPR()
{
    int i;
    char txt_regnum[6];
    char *txt_p[3];

    GPR_opened = 1;

    //CREATION TABLEAU REGISTRES R4300
    frGPR = gtk_frame_new("GPR");

    //tableGPR = gtk_table_new(32, 3, FALSE);
    clGPR = gtk_clist_new(3);
    for (i=0; i<3; i++) {
        gtk_clist_set_column_auto_resize(GTK_CLIST(clGPR), i, TRUE);
    }
    gtk_widget_modify_font(clGPR, debugger_font_desc);
    gtk_container_add( GTK_CONTAINER(frGPR), clGPR);

    txt_p[0] = txt_regnum;
    txt_p[1] = "MMMMMMMMMMMMMMMM";
    for(i=0; i<32; i++) {
        sprintf(txt_regnum, "%d", i);
        txt_p[2] = mnemonicGPR[i];
        gtk_clist_append(GTK_CLIST(clGPR), txt_p);
    }

    //Initialisation des registres fantomes.
    for( i=0; i<32; i++)
    {
        gui_fantom_gpr[i] = 0x1234567890LL; //la valeur la moins probable possible
    }
}




//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void update_GPR()
{
    int i;
    char txt[24];

    for(i=0; i<32; i++) {
        // Les "registres fantomes" evitent de raffraichir l'affichage de chaque
        // registre. Seules les modifications sont affichees a l'ecran.
        if(gui_fantom_gpr[i]!=reg[i]) {
            gui_fantom_gpr[i] = reg[i];
            sprintf(txt, "%.16llX", reg[i]);
            gtk_clist_set_text(GTK_CLIST(clGPR), i, 1, txt);
            gtk_clist_set_background( GTK_CLIST(clGPR), i, &color_modif);
        } else {
             gtk_clist_set_background( GTK_CLIST(clGPR), i, &color_ident);
        }
    }
}

