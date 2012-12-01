/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - breakpoints.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 HyperHacker                                        *
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "breakpoints.h"

#include "../../translate.h"
#include "../../main.h"

static int selected[BREAKPOINTS_MAX_NUMBER];

static void on_row_selection(GtkCList *clist, gint row);
static void on_row_unselection(GtkCList *clist, gint row);
static void on_add();
static void on_remove();
static void on_enable();
static void on_disable();
static void on_toggle();
static void _toggle(int flag);
static void on_edit();
static void on_close();

static GtkWidget *clBreakpoints;

static GdkColor color_BPEnabled, color_BPDisabled;

//]=-=-=-=-=-=-=-=-=-=-=-=[ Breakpoints Initialisation ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_breakpoints()
{
    int i;
    GtkWidget   *boxH1,
            *scrolledwindow1,
            *boxV1,
            *buAdd, *buRemove, *buEnable, *buDisable, *buToggle,
            *buEdit;

    breakpoints_opened = 1;

    for(i=0; i<BREAKPOINTS_MAX_NUMBER; i++) {
        selected[i]=0;
    }

    //=== Creation of Breakpoints Management ===========/
    winBreakpoints = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW(winBreakpoints), "Breakpoints");
    gtk_window_set_default_size( GTK_WINDOW(winBreakpoints), 100, 160);
    gtk_container_set_border_width( GTK_CONTAINER(winBreakpoints), 2);

    boxH1 = gtk_hbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER(winBreakpoints), boxH1 );

    //=== Creation of Breakpoints Display ==============/
    scrolledwindow1 = gtk_scrolled_window_new( NULL, NULL );
    gtk_box_pack_start( GTK_BOX(boxH1), scrolledwindow1, FALSE, FALSE, 0);
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolledwindow1),
                    GTK_POLICY_NEVER,
                    GTK_POLICY_AUTOMATIC );
    gtk_range_set_update_policy( GTK_RANGE (GTK_SCROLLED_WINDOW(scrolledwindow1)->hscrollbar),
                    GTK_POLICY_AUTOMATIC );

    clBreakpoints = gtk_clist_new( 1 );
    gtk_container_add( GTK_CONTAINER(scrolledwindow1), clBreakpoints );
    gtk_clist_set_selection_mode( GTK_CLIST(clBreakpoints), GTK_SELECTION_EXTENDED );
    gtk_clist_set_column_width( GTK_CLIST(clBreakpoints), 0, 170 );
    gtk_clist_set_auto_sort( GTK_CLIST(clBreakpoints), TRUE );
    
    //=== Creation of the Buttons ======================/
    boxV1 = gtk_vbox_new( FALSE, 2 );
    gtk_box_pack_end( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0 );
    
    buAdd = gtk_button_new_with_label("Add");
    gtk_box_pack_start( GTK_BOX(boxV1), buAdd, FALSE, FALSE, 0 );
    
    buRemove = gtk_button_new_with_label( "Remove" );
    gtk_box_pack_start( GTK_BOX(boxV1), buRemove, FALSE, FALSE, 0 );
    
    buEnable = gtk_button_new_with_label( "Enable" );
    gtk_box_pack_start( GTK_BOX(boxV1), buEnable, FALSE, FALSE, 0 );
    
    buDisable = gtk_button_new_with_label( "Disable" );
    gtk_box_pack_start( GTK_BOX(boxV1), buDisable, FALSE, FALSE, 0 );
    
    buToggle = gtk_button_new_with_label( "Toggle" );
    gtk_box_pack_start( GTK_BOX(boxV1), buToggle, FALSE, FALSE, 0 );
    
    buEdit = gtk_button_new_with_label( "Edit" );
    gtk_box_pack_start( GTK_BOX(boxV1), buEdit, FALSE, FALSE, 0 );

    gtk_widget_show_all(winBreakpoints);

    //=== Signal Connections ===========================/
    gtk_signal_connect( GTK_OBJECT(clBreakpoints), "select-row", (GtkSignalFunc) on_row_selection, NULL );
    gtk_signal_connect( GTK_OBJECT(clBreakpoints), "unselect-row", (GtkSignalFunc) on_row_unselection, NULL );
    gtk_signal_connect( GTK_OBJECT(buAdd), "clicked", on_add, NULL );
    gtk_signal_connect( GTK_OBJECT(buRemove), "clicked", on_remove, NULL );
    gtk_signal_connect( GTK_OBJECT(buEnable), "clicked", on_enable, NULL );
    gtk_signal_connect( GTK_OBJECT(buDisable), "clicked", on_disable, NULL );
    gtk_signal_connect( GTK_OBJECT(buToggle), "clicked", on_toggle, NULL );
    gtk_signal_connect( GTK_OBJECT(buEdit), "clicked", on_edit, NULL );
    gtk_signal_connect( GTK_OBJECT(winBreakpoints), "destroy", on_close, NULL );

    color_BPEnabled.red=0xFFFF;
    color_BPEnabled.green=0x7A00;
    color_BPEnabled.blue=0x7A00;

    color_BPDisabled.red=0x7A00;
    color_BPDisabled.green=0x7A00;
    color_BPDisabled.blue=0x7A00;

    // update list in case breakpoints were previously defined
    update_breakpoints();
}


//]=-=-=-=-=-=-=-=-=-=-=-=[ Update Breakpoints Display ]=-=-=-=-=-=-=-=-=-=-=-=[

void get_breakpoint_display_string(char* buf, breakpoint* bpt)
{
    if(bpt->address == bpt->endaddr)
    {
        sprintf(buf, "%c%c%c%c 0x%08X",
            (bpt->flags & BPT_FLAG_READ) ? 'R' : '-',
            (bpt->flags & BPT_FLAG_WRITE) ? 'W' : '-',
            (bpt->flags & BPT_FLAG_EXEC) ? 'X' : '-',
            (bpt->flags & BPT_FLAG_LOG) ? 'L' : '-',
            bpt->address);
    }
    else
    {
        sprintf(buf, "%c%c%c%c 0x%08X - 0x%08X",
            (bpt->flags & BPT_FLAG_READ) ? 'R' : '-',
            (bpt->flags & BPT_FLAG_WRITE) ? 'W' : '-',
            (bpt->flags & BPT_FLAG_EXEC) ? 'X' : '-',
            (bpt->flags & BPT_FLAG_LOG) ? 'L' : '-',
            bpt->address, bpt->endaddr);
    }
}

void update_breakpoints( )
{
    char line[1][64];
    line[0][0] = 0;

    if(!breakpoints_opened) return;

    gtk_clist_freeze( GTK_CLIST(clBreakpoints) );
    gtk_clist_clear( GTK_CLIST(clBreakpoints) );
    int i;
    for( i=0; i < g_NumBreakpoints; i++)
    gtk_clist_append( GTK_CLIST(clBreakpoints), (gchar **) line );

    for( i=0; i < g_NumBreakpoints; i++ )
    {
        get_breakpoint_display_string(line[0], &g_Breakpoints[i]);
        //printf("%s\n", line[0]);
        gtk_clist_set_text( GTK_CLIST(clBreakpoints), i, 0, line[0] );
        if(BPT_CHECK_FLAG(g_Breakpoints[i], BPT_FLAG_ENABLED))
            gtk_clist_set_background( GTK_CLIST(clBreakpoints), i, &color_BPEnabled);
        else
            gtk_clist_set_background( GTK_CLIST(clBreakpoints), i, &color_BPDisabled);
    }
    gtk_clist_thaw( GTK_CLIST(clBreakpoints) );
}

void remove_breakpoint_by_row( int row )
{
    //uint32 address;
    
    //address = (uint32) gtk_clist_get_row_data( GTK_CLIST(clBreakpoints), row);
    
    //int i = lookup_breakpoint( address );

    remove_breakpoint_by_num( row );
    
    //gtk_clist_remove( GTK_CLIST(clBreakpoints), row);
    update_breakpoints();
    refresh_desasm();
}


//]=-=-=-=-=-=-=[ Les Fonctions de Retour des Signaux (CallBack) ]=-=-=-=-=-=-=[

static void on_row_selection(GtkCList *clist, gint row)
{
    selected[row]=1;
}


static void on_row_unselection(GtkCList *clist, gint row)
{
    selected[row]=0;
}


static gint modify_address(ClistEditData *ced, const gchar *old, const gchar *new, gpointer data)
{
    breakpoint newbp; //New breakpoint to be added
    int i, j;
    char line[64];

    //printf( "modify_address %lX \"%s\"\n", address, new);
    
    //Input format: "[*][r][w][x][l] address [endaddr]"
    //*: if present, disabled by default
    //r: if present, break on read
    //w: if present, break on write
    //x: if present, break on execute (if none of r, w, x is specified execute is the default)
    //l (L): if present, log to the console when this breakpoint hits.
    //address: where to break
    //endaddr: if specified, break on all addresses address <= x <= endaddr
    
    //Enabled by default
    newbp.flags = BPT_FLAG_ENABLED;
    newbp.address = 0;
    newbp.endaddr = 0;
    
    //Copy line, stripping dashes, so sscanf() will work for getting addresses when editing
    j = 0;
    for(i=0; new[i]; i++)
    {
        if(new[i] == '-') continue;
        line[j] = new[i];
        j++;
        if((j + 1) >= sizeof(line)) break;
    }
    line[j] = 0;
    
    //Read flags
    for(i=0; line[i]; i++)
    {
        if((line[i] == ' ') //if space,
        || ((line[i] >= '0') && (line[i] <= '9')) //number,
        || ((line[i] >= 'A') && (line[i] <= 'F')) //A-F,
        || ((line[i] >= 'a') && (line[i] <= 'f'))) break; //or a-f, address begins here.
        else if(line[i] == '*') newbp.flags &= ~BPT_FLAG_ENABLED;
        else if((line[i] == 'r') || (line[i] == 'R')) newbp.flags |= BPT_FLAG_READ;
        else if((line[i] == 'w') || (line[i] == 'W')) newbp.flags |= BPT_FLAG_WRITE;
        else if((line[i] == 'x') || (line[i] == 'X')) newbp.flags |= BPT_FLAG_EXEC;
        else if((line[i] == 'l') || (line[i] == 'L')) newbp.flags |= BPT_FLAG_LOG;
    }
    
    //If none of r/w/x specified, default to exec
    if(!(newbp.flags & (BPT_FLAG_EXEC | BPT_FLAG_READ | BPT_FLAG_WRITE)))
        BPT_SET_FLAG(newbp, BPT_FLAG_EXEC);
        
    //Read address
    //printf("Address \"%s\"\n", &line[i]);
    i = sscanf(&line[i], "%X %x", &newbp.address, &newbp.endaddr);
    if(!i)
    {
        error_message(tr("Invalid address."));
        return FALSE;
    }
    else if(i == 1) newbp.endaddr = newbp.address;
    
    if(breakpoints_editing)
    {
        //printf("Updating breakpoint #%u on 0x%08X - 0x%08X flags 0x%08X\n", ced->row, newbp.address, newbp.endaddr, newbp.flags);
        replace_breakpoint_num(ced->row, &newbp);
    }
    else
    {
        //printf("Adding breakpoint on 0x%08X - 0x%08X flags 0x%08X\n", newbp.address, newbp.endaddr, newbp.flags);
        if(add_breakpoint_struct(&newbp) == -1)
        {
            error_message(tr("Cannot add any more breakpoints."));
            return FALSE;
        }
    }
    
    gtk_clist_set_row_data( GTK_CLIST(ced->clist), ced->row, (gpointer)(long) newbp.address );
    
    update_breakpoints();
    refresh_desasm();
    return FALSE; //don't add the typed string, update_breakpoints() handles it
}

static void on_add()
{
    int new_row;    //index of the appended row.
    char **line;

    //fixme: hacks ahoy! 
    breakpoints_editing = 0;
    line = malloc(1*sizeof(char*));
    line[0] = malloc(64*sizeof(char));
    
    line[0] = (char*)malloc(64);
    //sprintf( line[0], "0x%lX", address);
    line[0] = 0;
    new_row = gtk_clist_append( GTK_CLIST(clBreakpoints), line );
    gtk_clist_set_text( GTK_CLIST(clBreakpoints), new_row, 0, (gpointer) line[0] );

    clist_edit_by_row(GTK_CLIST(clBreakpoints), new_row, 0, modify_address, NULL);
    free(line[0]);
    free(line);
    
    refresh_desasm();

/*
    if(add_breakpoint(address) == -1)
    {
    //TODO: warn max number of breakpoints reached
    return;
    }
    
    line = malloc(1*sizeof(char*));    // new breakpoint:
    line[0] = malloc(64*sizeof(char)); // - address
// TODO:    line[1] = malloc(16*sizeof(char)); // - enabled/disabled
    
    sprintf( line[0], "0x%lX", address);

    new_row = gtk_clist_append( GTK_CLIST(clBreakpoints), line );
    gtk_clist_set_text( GTK_CLIST(clBreakpoints), new_row, 0, (gpointer) line[0] );

    clist_edit_by_row(GTK_CLIST(clBreakpoints), new_row, 0, modify_address, NULL);
    update_breakpoints();
    //FIXME:color are not updated +everything
 */
}


static void on_remove()
{
    int i;
    uint32 address;

    if(!g_NumBreakpoints) return;
    gtk_clist_freeze( GTK_CLIST(clBreakpoints) );
    for( i=BREAKPOINTS_MAX_NUMBER-1; i>=0; i-- )
    {
        if( selected[i] == 1 ) {
            address = (uint32)(long) gtk_clist_get_row_data( GTK_CLIST(clBreakpoints), i);
            remove_breakpoint_by_row( i );
            selected[i] = 0;
        }
    }
    gtk_clist_unselect_all( GTK_CLIST(clBreakpoints) );
    gtk_clist_thaw( GTK_CLIST(clBreakpoints) );
    refresh_desasm();
}


static void on_enable()
{
    _toggle(1);
}

static void on_disable()
{
    _toggle(0);
}


static void on_toggle()
{
    _toggle(-1);
}

//flag is 1 for enable, 0 for disable, -1 for toggle
static void _toggle(int flag)
{
    int i;
    
    if(!g_NumBreakpoints) return;
    for( i=BREAKPOINTS_MAX_NUMBER-1; i>=0; i-- )
    {
        if( selected[i] == 1 )
        {
            if(flag == 1) enable_breakpoint(i);
            else if(flag == 0) disable_breakpoint(i);
            else if(BPT_CHECK_FLAG(g_Breakpoints[i], BPT_FLAG_ENABLED)) disable_breakpoint(i);
            else enable_breakpoint(i);
            selected[i] = 0;
        }
    }
    update_breakpoints();
    refresh_desasm();
}


static void on_edit()
{
    int i, row = -1;
    //FIXME: not yet implemented. should display the textbox again as if we were entering
    //a new breakpoint, and update the selected one. probably disable the button if more
    //than one or none selected.
    
    for(i=0; i < g_NumBreakpoints; i++)
    {
        if(selected[i])
        {
            row = i;
            break;
        }
    }
    if(row == -1) return;
    breakpoints_editing = 1;
    clist_edit_by_row(GTK_CLIST(clBreakpoints), row, 0, modify_address, NULL);
    
    refresh_desasm();
}


static void on_close()
{
    breakpoints_opened = 0;
}

