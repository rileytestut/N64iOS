/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regCop1.c                                               *
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

#include "regCop1.h"

static float    gui_fantom_simple[32];
static double   gui_fantom_double[32];

GtkWidget   *clFGR;
GtkWidget   *clFGR2;

//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

static GtkWidget * init_FGR_col()
{
    GtkCList *cl;
    int i;
    char *txt[2];
    char txt_regnum[6];
    
    txt[0] = txt_regnum;
    txt[1] = "MMMMMMMMMM";
    
    cl = (GtkCList *) gtk_clist_new(2);
    gtk_widget_modify_font(GTK_WIDGET(cl), debugger_font_desc);
    gtk_clist_set_column_resizeable(cl, 0, FALSE);
    gtk_clist_set_column_resizeable(cl, 1, FALSE);
    for (i=0; i<32; i++) {
        sprintf(txt_regnum, "%d", i);
        gtk_clist_append(cl, txt);
    }
    gtk_clist_set_column_width(cl, 0, gtk_clist_optimal_column_width(cl, 0));
    gtk_clist_set_column_width(cl, 1, gtk_clist_optimal_column_width(cl, 1));
    
    return GTK_WIDGET(cl);
}

void init_FGR()
{
    GtkWidget *boxH1;
    int i;

    FGR_opened = 1;

    frFGR = gtk_frame_new("cop1");

    boxH1 = gtk_hbox_new( FALSE, 2);
    gtk_container_add( GTK_CONTAINER(frFGR), boxH1 );
    gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5 );

    //==== Simple Precision Registers Display ============/
    clFGR = init_FGR_col();
    gtk_box_pack_start( GTK_BOX(boxH1), clFGR, TRUE, TRUE, 0);

    //==== Double Precision Registers Display ============/
    clFGR2 = init_FGR_col();
    gtk_box_pack_start( GTK_BOX(boxH1), clFGR2, TRUE, TRUE, 0);

    //=== Fantom Registers Initialisation ============/
    for( i=0; i<32; i++) {
        gui_fantom_simple[i] = 1,2345678; // Some improbable value
        gui_fantom_double[i] = 9,8765432;
    }
}

//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop1 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_FGR()
{
    int i;
    char txt[24];

    gtk_clist_freeze( GTK_CLIST(clFGR) );
    for(i=0; i<32; i++) {
        if( gui_fantom_simple[i]!= *reg_cop1_simple[i] )
        {
            gui_fantom_simple[i] = *reg_cop1_simple[i];
            sprintf(txt, "%f", *reg_cop1_simple[i] );
            gtk_clist_set_text( GTK_CLIST(clFGR), i, 1, txt );
            gtk_clist_set_background( GTK_CLIST(clFGR), i, &color_modif);
        } else {
            gtk_clist_set_background( GTK_CLIST(clFGR), i, &color_ident);
        }
    }
    gtk_clist_thaw( GTK_CLIST(clFGR) );

    gtk_clist_freeze( GTK_CLIST(clFGR2) );
    for(i=0; i<32; i++) {
        if( gui_fantom_double[i]!=  *reg_cop1_double[i] )
        {
            gui_fantom_double[i] = *reg_cop1_double[i];
            sprintf(txt, "%f", *reg_cop1_double[i] );
            gtk_clist_set_text( GTK_CLIST(clFGR2), i, 1, txt );
            gtk_clist_set_background( GTK_CLIST(clFGR2), i, &color_modif);
        } else {
            gtk_clist_set_background( GTK_CLIST(clFGR2), i, &color_ident);
        }
    }
    gtk_clist_thaw( GTK_CLIST(clFGR2) );
}

