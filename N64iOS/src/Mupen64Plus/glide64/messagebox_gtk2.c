/***************************************************************************
                          messagebox.c  -  description
                             -------------------
    begin                : Tue Nov 12 2002
    copyright            : (C) 2002 by blight
    email                : blight@Ashitaka
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "messagebox.h"
#include "support.h"

#include <gtk/gtk.h>

#include <stdarg.h>
#include <stdio.h>

// include icons
#include "icons/messagebox-error.xpm"
#include "icons/messagebox-info.xpm"
#include "icons/messagebox-quest.xpm"
#include "icons/messagebox-warn.xpm"

static gint delete_question_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   return TRUE; // undeleteable
}

static void
button1_clicked( GtkWidget *widget, gpointer data )
{
    int *ret = (int *)data;

    *ret = 1;
}

static void
button2_clicked( GtkWidget *widget, gpointer data )
{
    int *ret = (int *)data;

    *ret = 2;
}

static void
button3_clicked( GtkWidget *widget, gpointer data )
{
    int *ret = (int *)data;

    *ret = 3;
}

int
messagebox( const char *title, int flags, const char *fmt, ... )
{
    va_list ap;
    char buf[2049];
    int ret = 0;

    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *icon = NULL;
    GtkWidget *label;
    GtkWidget *button1, *button2 = NULL, *button3 = NULL;

    va_start( ap, fmt );
    vsnprintf( buf, 2048, fmt, ap );
    va_end( ap );

    // check flags
    switch( flags & 0x000000FF )
    {
    case MB_ABORTRETRYIGNORE:
        button1 = gtk_button_new_with_label( "Abort" );
        button2 = gtk_button_new_with_label( "Retry" );
        button3 = gtk_button_new_with_label( "Ignore" );
        break;

    case MB_CANCELTRYCONTINUE:
        button1 = gtk_button_new_with_label( "Cancel" );
        button2 = gtk_button_new_with_label( "Retry" );
        button3 = gtk_button_new_with_label( "Continue" );
        break;

    case MB_OKCANCEL:
        button1 = gtk_button_new_with_label( "Ok" );
        button2 = gtk_button_new_with_label( "Cancel" );
        break;

    case MB_RETRYCANCEL:
        button1 = gtk_button_new_with_label( "Retry" );
        button2 = gtk_button_new_with_label( "Cancel" );
        break;

    case MB_YESNO:
        button1 = gtk_button_new_with_label( "Yes" );
        button2 = gtk_button_new_with_label( "No" );
        break;

    case MB_YESNOCANCEL:
        button1 = gtk_button_new_with_label( "Yes" );
        button2 = gtk_button_new_with_label( "No" );
        button3 = gtk_button_new_with_label( "Cancel" );
        break;

    case MB_OK:
    default:
        button1 = gtk_button_new_with_label( "Ok" );
    }

    dialog = gtk_dialog_new();
    gtk_container_set_border_width( GTK_CONTAINER(dialog), 10 );
    gtk_window_set_title( GTK_WINDOW(dialog), title );
    gtk_window_set_policy( GTK_WINDOW(dialog), 0, 0, 0 );
    gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
                GTK_SIGNAL_FUNC(delete_question_event), (gpointer)NULL );

    hbox = gtk_hbox_new( FALSE, 5 );
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0 );
    gtk_widget_show( hbox );

    // icon
    switch( flags & 0x00000F00 )
    {
    case MB_ICONWARNING:
        icon = create_pixmap_d( dialog, messagebox_warn_xpm );
        break;

    case MB_ICONINFORMATION:
        icon = create_pixmap_d( dialog, messagebox_info_xpm );
        break;

    case MB_ICONQUESTION:
        icon = create_pixmap_d( dialog, messagebox_quest_xpm );
        break;

    case MB_ICONERROR:
        icon = create_pixmap_d( dialog, messagebox_error_xpm );
        break;
    }

    if( icon )
    {
        gtk_box_pack_start( GTK_BOX(hbox), icon, FALSE, FALSE, 0 );
        gtk_widget_show( icon );
    }

    label = gtk_label_new( buf );
    gtk_box_pack_start( GTK_BOX(hbox), label, TRUE, TRUE, 0 );
    gtk_widget_show( label );

    if( button1 )
    {
        gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area), button1, TRUE, TRUE, 0 );
        gtk_widget_show( button1 );
        gtk_signal_connect( GTK_OBJECT(button1), "clicked",
                    GTK_SIGNAL_FUNC(button1_clicked), (gpointer)&ret );

    }
    if( button2 )
    {
        gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area), button2, TRUE, TRUE, 0 );
        gtk_widget_show( button2 );
        gtk_signal_connect( GTK_OBJECT(button2), "clicked",
                    GTK_SIGNAL_FUNC(button2_clicked), (gpointer)&ret );
    }
    if( button3 )
    {
        gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dialog)->action_area), button3, TRUE, TRUE, 0 );
        gtk_widget_show( button3 );
        gtk_signal_connect( GTK_OBJECT(button3), "clicked",
                    GTK_SIGNAL_FUNC(button3_clicked), (gpointer)&ret );
    }

    gtk_widget_show( dialog );

    while( !ret )
        if( gtk_main_iteration() )
            break;

    gtk_widget_destroy( dialog );

    return ret;
}

