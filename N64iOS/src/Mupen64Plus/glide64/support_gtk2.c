/***************************************************************************
                          support.c  -  description
                             -------------------
    begin                : Fri Nov 8 2002
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

#include "support.h"

#include <gtk/gtk.h>

// function to create pixmaps from buffers
GtkWidget *
create_pixmap_d                        (GtkWidget       *widget,
                                        const gchar     **data)
{
        GdkColormap *colormap;
        GdkPixmap *gdkpixmap;
        GdkBitmap *mask;
        GtkWidget *pixmap;

        colormap = gtk_widget_get_colormap (widget);
        gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, (gchar**) data);
        pixmap = gtk_pixmap_new (gdkpixmap, mask);
        gdk_pixmap_unref (gdkpixmap);
        gdk_bitmap_unref (mask);

        return pixmap;
}

