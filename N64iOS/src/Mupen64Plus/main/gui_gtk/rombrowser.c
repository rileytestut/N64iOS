/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rombrowser.c                                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Blight                                              *
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

/* rombrowser.c - Handles rombrowser GUI elements */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gtk/gtk.h>

#include "configdialog.h"
#include "rombrowser.h"
#include "romproperties.h"
#include "main_gtk.h"

#include "../main.h" 
#include "../util.h"
#include "../config.h"
#include "../translate.h"
#include "../rom.h"

/* Globals. */
GdkPixbuf *australia, *europe, *france, *germany, *italy, *japan, *spain, *usa, *japanusa, *n64cart, *star, *staroff;

/*********************************************************************************************************
* Rombrowser functions.
*/
void countrycodeflag(unsigned short int countrycode, GdkPixbuf** flag)
{
    switch(countrycode)
    {
    case 0x41: /* Japan / USA */
        *flag = japanusa;
        break;

    case 0x44: /* Germany */
        *flag = germany;
        break;

    case 0x45: /* USA */
        *flag = usa;
        break;

    case 0x46: /* France */
        *flag = france;
        break;

    case 'I':  /* Italy */
        *flag = italy;
        break;

    case 0x4A: /* Japan */
        *flag = japan;
        break;

    case 'S':  /* Spain */
        *flag = spain;
        break;

    case 0x55: case 0x59:  /* Australia */
        *flag = australia;
        break;

    case 0x50: case 0x58: case 0x20:
    case 0x21: case 0x38: case 0x70:
        *flag = europe;
        break;

    default:
        *flag = n64cart;
        break;
    }
}

char* filefrompath(const char* string)
{
    int stringlength, bufferlength, counter;
    char* buffer;

    stringlength = strlen(string);

    for(counter = stringlength; counter > 0; --counter)
        {
        if(string[counter]=='/')
            {
            ++counter;
            break;
            }
        }

    bufferlength = stringlength-counter;

    buffer = (char*)malloc((bufferlength+1)*sizeof(char));
    strncpy(buffer, string+counter, bufferlength);
    buffer[bufferlength] = '\0';

    return buffer;
}


/* Dummy compare function for use when updating model. As sorting is not currently toggable in gtk. */
static gint return_zero(GtkTreeModel* model, GtkTreeIter* ptr1, GtkTreeIter* ptr2, gpointer user_data)
{
    return 0;
}

static gint rombrowser_compare(GtkTreeModel* model, GtkTreeIter* ptr1, GtkTreeIter* ptr2, gpointer user_data)
{
    short returnvalue;

    if(g_MainWindow.romSortColumn==2||g_MainWindow.romSortColumn==11||g_MainWindow.romSortColumn==14)
        {
        cache_entry* entry1;
        cache_entry* entry2;
        int item1, item2;

        gtk_tree_model_get(model, ptr1, 22, &entry1, -1);
        gtk_tree_model_get(model, ptr2, 22, &entry2, -1);

        if(g_MainWindow.romSortColumn==2)
            {
            item1 = entry1->inientry->status;
            item2 = entry2->inientry->status;
            }
        else if(g_MainWindow.romSortColumn==11)
            {
            item1 = entry1->romsize;
            item2 = entry2->romsize;
            }
        else
            {
            item1 = entry1->cic;
            item2 = entry2->cic;
            }

        if(item1==item2)
            returnvalue = 0;
        else if(item1>item2)
            returnvalue = -1;
        else
            returnvalue = 1;
        }
    else
        {
        gchar *buffer1, *buffer2;

        gtk_tree_model_get(model, ptr1, g_MainWindow.romSortColumn, &buffer1, -1);
        gtk_tree_model_get(model, ptr2, g_MainWindow.romSortColumn, &buffer2, -1);

        if(buffer1==NULL&&buffer2==NULL)
            returnvalue = 0;
        else if(buffer1==NULL)
            returnvalue = 1;
        else if(buffer2==NULL)
            returnvalue = -1;
        else
            returnvalue = strcasecmp(buffer1, buffer2);

        if(buffer1)
            g_free(buffer1);
        if(buffer2)
            g_free(buffer2);
        }

    /* If items are equal, fallback on Good Name and then File Name. */
    if(returnvalue==0&&g_MainWindow.romSortColumn!=1)
        {
        gchar *buffer1, *buffer2;

        gtk_tree_model_get(model, ptr1, 1, &buffer1, -1);
        gtk_tree_model_get(model, ptr2, 1, &buffer2, -1);

        if(buffer1==NULL&&buffer2 ==NULL)
            returnvalue = 0;
        else if(buffer1==NULL)
            returnvalue = 1;
        else if(buffer2==NULL)
            returnvalue = -1;
        else
            returnvalue = strcasecmp(buffer1, buffer2);

        if(buffer1)
            g_free(buffer1);
        if(buffer2)
            g_free(buffer2);
        }

    if(returnvalue==0&&g_MainWindow.romSortColumn!=4)
        {
        gchar *buffer1, *buffer2;

        gtk_tree_model_get(model, ptr1, 4, &buffer1, -1);
        gtk_tree_model_get(model, ptr2, 4, &buffer2, -1);

        if(buffer1==NULL&&buffer2==NULL)
            returnvalue = 0;
        else if(buffer1==NULL)
            returnvalue = 1;
        else if(buffer2==NULL)
            returnvalue = -1;
        else
            returnvalue = strcasecmp(buffer1, buffer2);

        if(buffer1)
            g_free(buffer1);
        if(buffer2)
            g_free(buffer2);
        }

    return returnvalue;
}

/* Load a GtkTreeModel after re-scanning directories. */
void rombrowser_refresh(unsigned int roms, unsigned short clear)
{
    int arrayroms;
    GtkTreeModel* model;

    /* If clear flag is set, clear the models. */
    if(clear)
        {
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romFullList));
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romFullList));
        gtk_tree_selection_select_all(selection);
        gtk_list_store_clear(GTK_LIST_STORE(model));

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));
        gtk_tree_selection_select_all(selection);
        gtk_list_store_clear(GTK_LIST_STORE(model));
        }

    model =  gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romFullList));
    arrayroms = gtk_tree_model_iter_n_children(model, NULL); 

    /* If there are currently more ROMs in cache than in the rombrowser, add them. */
    if(roms>arrayroms)
        {
        gboolean fullpaths;

        GtkTreeIter* iter = (GtkTreeIter*)malloc(sizeof(GtkTreeIter));
        char *country, *goodname, *usercomments, *md5hash, *crc1, *crc2, *internalname, *savetype, *players, *size, *compressiontype, *imagetype, *cicchip, *rumble;
        char* filename = NULL; /* Needs to be NULLed if fullpaths and zero roms are updated, for free. */
        GdkPixbuf* flag;
        GdkPixbuf* status[5];
        unsigned int romcounter;
        int counter;

        country = (char*)calloc(32, sizeof(char));
        md5hash = (char*)calloc(33, sizeof(char));
        crc1 = (char*)calloc(9, sizeof(char));
        crc2 = (char*)calloc(9, sizeof(char));
        savetype = (char*)calloc(16, sizeof(char));
        players = (char*)calloc(16, sizeof(char));
        size = (char*)calloc(16, sizeof(char));
        compressiontype = (char*)calloc(16, sizeof(char));
        imagetype = (char*)calloc(32, sizeof(char));
        cicchip = (char*)calloc(16, sizeof(char));
        rumble = (char*)calloc(8, sizeof(char));

        if(iter==NULL||country==NULL||md5hash==NULL||crc1==NULL||crc2==NULL||size==NULL||compressiontype==NULL||imagetype==NULL||cicchip==NULL)
            {
            fprintf(stderr, "%s, %d: Out of memory!\n", __FILE__, __LINE__); 
            return;
            }

        fullpaths = config_get_bool("RomBrowserShowFullPaths", FALSE);
        cache_entry* entry;
        entry = g_romcache.top;

        /* Advance cache pointer. */
        for(romcounter=0; romcounter < arrayroms; ++romcounter)
            {
            entry = entry->next;
            if(entry==NULL)
                return;
            }

        for(romcounter=0; (romcounter < roms) && (entry != NULL); ++romcounter)
          {
          countrycodestring(entry->countrycode, country);
          countrycodeflag(entry->countrycode, &flag);
          goodname = entry->inientry->goodname;

          for(counter = 0; counter < 5; ++counter)
                {
                if(entry->inientry->status>counter)
                    status[counter] = star;
                else
                    status[counter] = staroff;
                }
            usercomments = entry->usercomments;
            if(fullpaths)
                filename = entry->filename;
            else
                filename = filefrompath(entry->filename);

            for(counter = 0; counter < 16; ++counter) 
                sprintf(md5hash+counter*2, "%02X", entry->md5[counter]);
            sprintf(crc1, "%08X", entry->crc1);
            sprintf(crc2, "%08X", entry->crc2);
            internalname=entry->internalname;
            savestring(entry->inientry->savetype, savetype);
            playersstring(entry->inientry->players, players);
            sprintf(size, "%.1f MBits", (float)(entry->romsize / (float)0x20000));
            compressionstring(entry->compressiontype, compressiontype);
            imagestring(entry->imagetype, imagetype);
            cicstring(entry->cic, cicchip);
            rumblestring(entry->inientry->rumble, rumble);

            /* Actually add entries to models. */
            model =  gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romFullList));
            gtk_list_store_append(GTK_LIST_STORE(model), iter);

            gtk_list_store_set(GTK_LIST_STORE(model), iter, 0, country, 1, goodname, 2, NULL, 3, usercomments, 4, filename, 5, md5hash, 6, crc1, 7, crc2, 8, internalname, 9, savetype, 10, players, 11, size, 12, compressiontype, 13, imagetype, 14, cicchip, 15, rumble, 16, status[0], 17, status[1], 18, status[2], 19, status[3], 20, status[4], 21, flag, 22, entry, -1);

            model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));
            gtk_list_store_append(GTK_LIST_STORE(model), iter);
            gtk_list_store_set(GTK_LIST_STORE(model), iter, 0, country, 1, goodname, 2, NULL, 3, usercomments, 4, filename, 5, md5hash, 6, crc1, 7, crc2, 8, internalname, 9, savetype, 10, players, 11, size, 12, compressiontype, 13, imagetype, 14, cicchip, 15, rumble, 16, status[0], 17, status[1], 18, status[2], 19, status[3], 20, status[4], 21, flag, 22, entry, -1);

            /*printf("Added: %s\n", goodname);*/
            entry = entry->next;
            }

        free(country);
        free(md5hash);
        free(crc1);
        free(crc2);
        free(savetype);
        free(players);
        free(size);
        free(compressiontype);
        free(imagetype);
        free(cicchip);
        free(rumble);
        if(!fullpaths)
           {
           if(filename!=NULL)
              free(filename);
           }
       free(iter);

        /* Do an initial sort. */
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), g_MainWindow.romSortColumn, rombrowser_compare, (gpointer)NULL, (gpointer)NULL);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), g_MainWindow.romSortColumn, g_MainWindow.romSortType);
        gtk_tree_view_set_model(GTK_TREE_VIEW(g_MainWindow.romDisplay), model);
        }
}

/*********************************************************************************************************
* Filter functions.
*/
/* We do our own filtering, but this is a GtkTreeModelFilter function.
 * We do filtering manually since GtkTreeModelFilter can not implement automatic
 * sorting and GtkTreeModelSort can't implement automatic filtering. It is likely
 * gtk will support this is the future.
 */
static gboolean filter_function(GtkTreeModel* model, GtkTreeIter* iter, gpointer data)
{
    const gchar* filter;
    filter = gtk_entry_get_text(GTK_ENTRY(g_MainWindow.filter));
    /* printf("Filter: %s\n", filter); */

    if(g_MainWindow.romSortColumn==2)
        {
        cache_entry* entry;
        gtk_tree_model_get(model, iter, 22, &entry, -1);

        if(filter[0]=='\0')
            return TRUE;

        if(atoi(filter)==entry->inientry->status)
            return TRUE;
        else
            return FALSE;
        }

    char *buffer1, *buffer2;
    gboolean returnvalue;

    gtk_tree_model_get(model, iter, g_MainWindow.romSortColumn, &buffer1, -1);
    /* printf("Value %s\n", buffer1); */
    if(buffer1==NULL||filter==NULL)
        returnvalue = TRUE;
    else
        {
        buffer2 = strcasestr(buffer1, filter);
        if(buffer2!=NULL)
            returnvalue =  TRUE;
       else
            returnvalue =  FALSE;
       }

    free(buffer1);
    return returnvalue;
}

/* Apply filter manually by copying iters from romFullList model to romDisplay model
 * if they match filter criteria. Optimized to just do a sort on romDisplaymodel if
 * filter is empty twice in a row.
 */
static void apply_filter()
{
    int g_iNumRoms;
    static short resort = 0;
    GtkTreeModel* destination;
    const gchar* filter;

    filter = gtk_entry_get_text(GTK_ENTRY(g_MainWindow.filter));
    if(filter[0]!='\0'||resort==1)
        {
        if(filter[0]!='\0')
            resort=1;
        else
            resort=0;

        GtkTreeModel *model, *source;
        GtkTreeIter sourceiter, destinationiter;
        gboolean validiter;

        char *country, *goodname, *usercomments, *filename, *md5hash, *crc1, *crc2, *internalname, *savetype, *players, *size, *compressiontype, *imagetype, *cicchip, *rumble;
        GdkPixbuf *status[5];
        GdkPixbuf *flag;
        cache_entry* entry; 
        short int counter;

        /* Clear the display model. */
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));
        gtk_tree_selection_select_all(selection);
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));
        gtk_list_store_clear(GTK_LIST_STORE(model));

        source = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romFullList));
        g_iNumRoms = gtk_tree_model_iter_n_children(source, NULL);
        validiter = gtk_tree_model_get_iter_first(source, &sourceiter);

        destination = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(destination), g_MainWindow.romSortColumn,  return_zero, (gpointer)NULL, (gpointer)NULL);

        if(validiter)
            {
            for(counter = 0; counter < g_iNumRoms; ++counter)
                {
                if(filter_function(source, &sourceiter, (gpointer)NULL))
                    {
                    gtk_tree_model_get(GTK_TREE_MODEL(source), &sourceiter, 0, &country, 1, &goodname, 3, &usercomments, 4, &filename, 5, &md5hash, 6, &crc1, 7, &crc2, 8, &internalname, 9, &savetype, 10, &players, 11, &size, 12, &compressiontype, 13, &imagetype, 14, &cicchip, 15, &rumble, 16, &status[0], 17, &status[1], 18, &status[2], 19, &status[3], 20, &status[4], 21, &flag, 22, &entry, -1);

                    gtk_list_store_append(GTK_LIST_STORE(destination), &destinationiter);
                    gtk_list_store_set(GTK_LIST_STORE(destination), &destinationiter, 0, country, 1, goodname, 2, NULL, 3, usercomments, 4, filename, 5, md5hash, 6, crc1, 7, crc2, 8, internalname, 9, savetype, 10, players, 11, size, 12, compressiontype, 13, imagetype, 14, cicchip, 15, rumble, 16, status[0], 17, status[1], 18, status[2], 19, status[3], 20, status[4], 21, flag, 22, entry, -1);
                    }
                if(!gtk_tree_model_iter_next(source, &sourceiter))
                     break;
                }
           }
        }
    else
        destination = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));

        /* Always sort romDisplay, otherwise regular column sorting won't work. */
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(destination), g_MainWindow.romSortColumn,  rombrowser_compare, (gpointer)NULL, (gpointer)NULL);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(destination), g_MainWindow.romSortColumn, g_MainWindow.romSortType);
}

/*********************************************************************************************************
* callbacks
*/
/* Activate filter widget -> filter and resort. */
static void callback_apply_filter(GtkWidget* widget, gpointer data)
{
    apply_filter();
}

/* Rombrowser right click context menu. */
gboolean callback_rombrowser_context(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));

    if(event->type==GDK_BUTTON_PRESS)
        {
        if(event->button==3) /* Right click. */
            {
            if(gtk_tree_selection_count_selected_rows(selection)>0)
                {
                gtk_widget_set_sensitive(g_MainWindow.playRomItem, TRUE);
                gtk_widget_set_sensitive(g_MainWindow.romPropertiesItem, TRUE);
                }
            else
                {
                gtk_widget_set_sensitive(g_MainWindow.playRomItem, FALSE);
                gtk_widget_set_sensitive(g_MainWindow.romPropertiesItem, FALSE);
                }
            gtk_menu_popup(GTK_MENU(data), NULL, NULL, NULL, NULL,
            event->button, event->time);

            return TRUE;
            }
        }

    return FALSE;
}

static void callback_play_rom(GtkWidget* widget, gpointer data)
{
    GList *list = NULL, *llist = NULL;
    cache_entry* entry;
    GtkTreeIter iter;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));

    list = gtk_tree_selection_get_selected_rows (selection, &model);

    if(!list) /* Should never happen since the item is only active when a row is selected. */
        return;

    llist = g_list_first(list);

    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)llist->data);
    gtk_tree_model_get(model, &iter, 22, &entry, -1);

    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(list);

    if(open_rom(entry->filename, entry->archivefile)==0)
        startEmulation();
}

static void callback_rom_properties(GtkWidget* widget, gpointer data)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_MainWindow.romDisplay));

    if (gtk_tree_selection_count_selected_rows(selection) > 0)
        {
        GList *list = NULL, *llist = NULL;
        list = gtk_tree_selection_get_selected_rows(selection, &model);
        llist = g_list_first(list);

        gtk_tree_model_get_iter(model, &g_RomPropDialog.iter,(GtkTreePath*) llist->data);
        gtk_tree_model_get(model, &g_RomPropDialog.iter, 22, &g_RomPropDialog.entry, -1);

        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);

        show_rom_properties();
        }
}

static void callback_configure_rombrowser(GtkWidget* widget, gpointer data)
{
    show_configure();
    gtk_notebook_set_current_page(GTK_NOTEBOOK(g_ConfigDialog.notebook), 2);
}

static void callback_call_rcs(GtkWidget* widget, gpointer data)
{
    g_romcache.rcstask = RCS_RESCAN;
}

static void callback_header_clicked(GtkWidget* widget, GdkEventButton* event, gpointer column)
{
    if(event->type==GDK_BUTTON_PRESS)
        {
        if(event->button==3) /* Right click. */
            {
            gtk_menu_popup(GTK_MENU(g_MainWindow.romHeaderMenu), NULL, NULL, NULL, NULL,
            event->button, event->time);
            }
        else if(event->button==1) /* Left click. */
            {
            if(g_MainWindow.romSortColumn==gtk_tree_view_column_get_sort_column_id(column))
                g_MainWindow.romSortType = (g_MainWindow.romSortType==GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
            else
                {
                g_MainWindow.romSortColumn = gtk_tree_view_column_get_sort_column_id(column);
                g_MainWindow.romSortType = GTK_SORT_ASCENDING;
                }

            config_put_number("RomSortColumn", g_MainWindow.romSortColumn);
            config_put_number("RomSortType", g_MainWindow.romSortType);
            apply_filter();
            }
        }
}

static void callback_column_visible(GtkWidget* widget, int column)
{
    int counter;

    gboolean visible = gtk_tree_view_column_get_visible(GTK_TREE_VIEW_COLUMN(g_MainWindow.column[column]));
    gtk_tree_view_column_set_visible(g_MainWindow.column[column], !visible);

    config_put_bool(g_MainWindow.column_names[column][1], !visible); 

    /* Control if emptry header column is visible. */
    gtk_tree_view_column_set_visible(g_MainWindow.column[16], FALSE);
    for(counter = 0; counter < 16; ++counter)
        {
        if(gtk_tree_view_column_get_visible(GTK_TREE_VIEW_COLUMN(g_MainWindow.column[counter])))
            return;
        }
    gtk_tree_view_column_set_visible(g_MainWindow.column[16], TRUE);
}

static gboolean callback_filter_selected(GtkWidget* widget, gpointer data)
{
    if(g_MainWindow.accelUnsafeActive)
        {
        gtk_window_remove_accel_group(GTK_WINDOW(g_MainWindow.window), g_MainWindow.accelUnsafe);
        g_MainWindow.accelUnsafeActive = FALSE;
        }
    return FALSE;
}

static gboolean callback_filter_unselected(GtkWidget* widget, gpointer data)
{
    if(!g_MainWindow.accelUnsafeActive)
        {
        gtk_window_add_accel_group(GTK_WINDOW(g_MainWindow.window), g_MainWindow.accelUnsafe);
        g_MainWindow.accelUnsafeActive = TRUE;
        }
    return FALSE;
}

static gboolean callback_filter_grab_unselected(GtkWidget* widget, gpointer data)
{
    if(!g_MainWindow.accelUnsafeActive)
        {
        gtk_window_add_accel_group(GTK_WINDOW(g_MainWindow.window), g_MainWindow.accelUnsafe);
        g_MainWindow.accelUnsafeActive = TRUE;
        }
    gtk_window_set_focus(GTK_WINDOW(g_MainWindow.window), NULL);
    return FALSE;
}

static void setup_view(GtkWidget* view)
{
    GtkListStore* store = gtk_list_store_new (23,
    G_TYPE_STRING,   /* 0 Country */
    G_TYPE_STRING,   /* 1 Good Name */
    GDK_TYPE_PIXBUF, /* 2 NULL for Status handle / empty */
    G_TYPE_STRING,   /* 3 User Comments */
    G_TYPE_STRING,   /* 4 File Name */
    G_TYPE_STRING,   /* 5 MD5 Hash */
    G_TYPE_STRING,   /* 6 CRC1 */
    G_TYPE_STRING,   /* 7 CRC2 */
    G_TYPE_STRING,   /* 8 Internal Name */
    G_TYPE_STRING,   /* 9 Save Type */
    G_TYPE_STRING,   /* 10 Players */
    G_TYPE_STRING,   /* 11 Size */
    G_TYPE_STRING,   /* 12 Compression */
    G_TYPE_STRING,   /* 13 Image Type */
    G_TYPE_STRING,   /* 14 CIC Chip */
    G_TYPE_STRING,   /* 15 Rumble */
    GDK_TYPE_PIXBUF, /* 16-20 Pixbufs for Status stars. */
    GDK_TYPE_PIXBUF,
    GDK_TYPE_PIXBUF,
    GDK_TYPE_PIXBUF,
    GDK_TYPE_PIXBUF,
    GDK_TYPE_PIXBUF, /* 21 Flag */
    G_TYPE_POINTER,  /* 22 RCS Entry */
    -1);

    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;
    GtkWidget* item;
    char buffer[128];
    int i;
    unsigned char visible;

    /* Create country flag / string dual rendered cell. */
    renderer = gtk_cell_renderer_pixbuf_new();
    g_MainWindow.column[0] = gtk_tree_view_column_new();
    column = g_MainWindow.column[0];
    gtk_tree_view_column_set_title(column, g_MainWindow.column_names[0][0]); 
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", 21);
    g_object_set(renderer, "xpad", 5, NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_reorderable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 0);
    gtk_tree_view_insert_column(GTK_TREE_VIEW (view), column, 0);
    g_signal_connect(column->button, "button-press-event", G_CALLBACK(callback_header_clicked), column);

    renderer = gtk_cell_renderer_text_new();
    g_MainWindow.column[1] = gtk_tree_view_column_new_with_attributes(g_MainWindow.column_names[1][0], renderer, "text", 1, NULL);
    column = g_MainWindow.column[1];
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_reorderable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 1);
    gtk_tree_view_insert_column(GTK_TREE_VIEW (view), column, 1);
    g_signal_connect(column->button, "button-press-event", G_CALLBACK(callback_header_clicked), column);

    /* Status stars. */
    g_MainWindow.column[2] = gtk_tree_view_column_new();
    column = g_MainWindow.column[2];
    gtk_tree_view_column_set_title(column, g_MainWindow.column_names[2][0]); 
    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer, "xpad", 2, NULL);
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", 2);
    for(i = 0; i < 5; ++i)
        {
        renderer = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_column_pack_start(column, renderer, FALSE);
        gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", 16+i);
        }
    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer, "xpad", 2, NULL);
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", 2);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_reorderable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 2);
    gtk_tree_view_insert_column(GTK_TREE_VIEW (view), column, 2);
    g_signal_connect(column->button, "button-press-event", G_CALLBACK(callback_header_clicked), column);

    for(i = 3; i < 16; ++i)
        {
        renderer = gtk_cell_renderer_text_new();
        g_MainWindow.column[i] = gtk_tree_view_column_new_with_attributes(g_MainWindow.column_names[i][0], renderer, "text", i, NULL);
        column = g_MainWindow.column[i];
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_reorderable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, i);
        gtk_tree_view_insert_column(GTK_TREE_VIEW (view), column, i);
        g_signal_connect(column->button, "button-press-event", G_CALLBACK(callback_header_clicked), column);
        }

    renderer = gtk_cell_renderer_text_new ();
    g_MainWindow.column[16] = gtk_tree_view_column_new();
    column = g_MainWindow.column[16];
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(view), column, 2);
    g_signal_connect(column->button, "button-press-event", G_CALLBACK(callback_header_clicked), column);

    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));

    g_MainWindow.romHeaderMenu = gtk_menu_new();

    /* Toggle column visibility from config file, make menu. */
    for(i = 0; i < 16; ++i)
        {
        snprintf(buffer, sizeof(buffer), " %s", g_MainWindow.column_names[i][0]);
        item = gtk_check_menu_item_new_with_mnemonic(buffer); 
        if((visible=config_get_bool(g_MainWindow.column_names[i][1], 2))==2)
             {
             visible = (i<5) ? TRUE: FALSE;
             config_put_bool(g_MainWindow.column_names[i][1], visible);
             }
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), visible); 
        gtk_tree_view_column_set_visible(g_MainWindow.column[i], visible);
        gtk_menu_shell_append(GTK_MENU_SHELL(g_MainWindow.romHeaderMenu), item);
        g_signal_connect(item, "activate", G_CALLBACK(callback_column_visible), GUINT_TO_POINTER(i));
        }

    gtk_tree_view_column_set_visible(g_MainWindow.column[16], FALSE);
    for(i = 0; i < 16; ++i)
        {
        if(gtk_tree_view_column_get_visible(GTK_TREE_VIEW_COLUMN(g_MainWindow.column[i])))
            return;
        }
    gtk_tree_view_column_set_visible(g_MainWindow.column[16], TRUE);
}

/*********************************************************************************************************
* gui functions
*/
void create_filter()
{
    GtkToolItem* toolitem;
    GtkWidget* label;
    GtkWidget* hbox;

    hbox = gtk_hbox_new(FALSE, 0);
    toolitem = gtk_tool_item_new(); /* This is how gtk does more complex toolbar items. */
    gtk_tool_item_set_expand(toolitem, TRUE);

    g_MainWindow.filterBar = gtk_toolbar_new();
    gtk_toolbar_set_orientation(GTK_TOOLBAR(g_MainWindow.filterBar), GTK_ORIENTATION_HORIZONTAL);

    label = gtk_label_new_with_mnemonic(tr("F_ilter:"));
    g_MainWindow.filter = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_MainWindow.filter);
    gtk_entry_set_text(GTK_ENTRY(g_MainWindow.filter), "");

    g_signal_connect(g_MainWindow.filter, "changed", G_CALLBACK(callback_apply_filter), NULL);
    g_signal_connect(g_MainWindow.filter, "activate", G_CALLBACK(callback_apply_filter), NULL);
    g_signal_connect(g_MainWindow.filter, "focus-in-event", G_CALLBACK(callback_filter_selected), NULL);
    g_signal_connect(g_MainWindow.filter, "focus-out-event", G_CALLBACK(callback_filter_unselected), NULL);
    g_signal_connect(g_MainWindow.filter, "grab-notify", G_CALLBACK(callback_filter_grab_unselected), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), g_MainWindow.filter, TRUE, TRUE, 5);

    gtk_container_add(GTK_CONTAINER(toolitem), hbox);
    gtk_toolbar_insert(GTK_TOOLBAR(g_MainWindow.filterBar), toolitem, 0);

    gtk_box_pack_start(GTK_BOX(g_MainWindow.toplevelVBox), g_MainWindow.filterBar, FALSE, FALSE, 0);
}

void create_romBrowser()
{
    GtkWidget* menu;
    GtkWidget* submenu;
    GtkWidget* item;

    australia = gdk_pixbuf_new_from_file(get_iconpath("australia.png"), NULL);
    europe = gdk_pixbuf_new_from_file(get_iconpath("europe.png"), NULL);
    france = gdk_pixbuf_new_from_file(get_iconpath("france.png"), NULL);
    germany = gdk_pixbuf_new_from_file(get_iconpath("germany.png"), NULL);
    italy = gdk_pixbuf_new_from_file(get_iconpath("italy.png"), NULL);
    japan = gdk_pixbuf_new_from_file(get_iconpath("japan.png"), NULL);
    spain = gdk_pixbuf_new_from_file(get_iconpath("spain.png"), NULL);
    usa = gdk_pixbuf_new_from_file(get_iconpath("usa.png"), NULL);
    japanusa = gdk_pixbuf_new_from_file(get_iconpath("japanusa.png"), NULL);
    n64cart = gdk_pixbuf_new_from_file(get_iconpath("16x16/mupen64cart.png"), NULL);

    /* Setup rombrowser column names.
    g_MainWindow.column_names[column][0] is the translated column header.
    g_MainWindow.column_names[column][1] toggles visability in the config system. */
    snprintf(g_MainWindow.column_names[0][0], 128, tr("Country"));
    snprintf(g_MainWindow.column_names[0][1], 128, "ColumnCountryVisible");
    snprintf(g_MainWindow.column_names[1][0], 128, tr("Good Name"));
    snprintf(g_MainWindow.column_names[1][1], 128, "ColumnGoodNameVisible");
    snprintf(g_MainWindow.column_names[2][0], 128, tr("Status"));
    snprintf(g_MainWindow.column_names[2][1], 128, "ColumnStatusVisible");
    snprintf(g_MainWindow.column_names[3][0], 128, tr("User Comments"));
    snprintf(g_MainWindow.column_names[3][1], 128, "ColumnUserCommentsVisible");
    snprintf(g_MainWindow.column_names[4][0], 128, tr("File Name"));
    snprintf(g_MainWindow.column_names[4][1], 128, "ColumnFileNameVisible");
    snprintf(g_MainWindow.column_names[5][0], 128, tr("MD5 Hash"));
    snprintf(g_MainWindow.column_names[5][1], 128, "ColumnMD5HashVisible");
    snprintf(g_MainWindow.column_names[6][0], 128, tr("CRC1"));
    snprintf(g_MainWindow.column_names[6][1], 128, "ColumnCRC1Visible");
    snprintf(g_MainWindow.column_names[7][0], 128, tr("CRC2"));
    snprintf(g_MainWindow.column_names[7][1], 128, "ColumnCRC2Visible");
    snprintf(g_MainWindow.column_names[8][0], 128, tr("Internal Name"));
    snprintf(g_MainWindow.column_names[8][1], 128, "ColumnInternalNameVisible");
    snprintf(g_MainWindow.column_names[9][0], 128, tr("Save Types"));
    snprintf(g_MainWindow.column_names[9][1], 128, "ColumnSaveTypeVisible");
    snprintf(g_MainWindow.column_names[10][0], 128, tr("Players"));
    snprintf(g_MainWindow.column_names[10][1], 128, "ColumnPlayersVisible");
    snprintf(g_MainWindow.column_names[11][0], 128, tr("Size"));
    snprintf(g_MainWindow.column_names[11][1], 128, "ColumnSizeVisible");
    snprintf(g_MainWindow.column_names[12][0], 128, tr("Compression"));
    snprintf(g_MainWindow.column_names[12][1], 128, "ColumnCompressionVisible");
    snprintf(g_MainWindow.column_names[13][0], 128, tr("Image Type"));
    snprintf(g_MainWindow.column_names[13][1], 128, "ColumnImageTypeVisible");
    snprintf(g_MainWindow.column_names[14][0], 128, tr("CIC Chip"));
    snprintf(g_MainWindow.column_names[14][1], 128, "ColumnCICChipVisible");
    snprintf(g_MainWindow.column_names[15][0], 128, tr("Rumble"));
    snprintf(g_MainWindow.column_names[15][1], 128, "ColumnRumbleVisible");

    /* Setup rombrowser tree views. */
    g_MainWindow.romFullList = gtk_tree_view_new();
    g_MainWindow.romDisplay = gtk_tree_view_new();

    /* Multiple selections needed due to the way we clear the TreeViews. */
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romFullList));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_MainWindow.romDisplay));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    /* Initialize TreeViews - i.e. setup column info. models */
    setup_view(g_MainWindow.romFullList);
    setup_view(g_MainWindow.romDisplay);

    g_MainWindow.romScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_MainWindow.romScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* Add the Display TreeView into our scrolled window. */
    gtk_container_add(GTK_CONTAINER(g_MainWindow.romScrolledWindow), g_MainWindow.romDisplay);
    gtk_container_add(GTK_CONTAINER(g_MainWindow.toplevelVBox), g_MainWindow.romScrolledWindow);

    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(g_MainWindow.romDisplay), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(g_MainWindow.romDisplay), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(g_MainWindow.romDisplay), FALSE);

    /* Open on double click. */
    g_signal_connect(g_MainWindow.romDisplay, "row-activated", G_CALLBACK(callback_play_rom), NULL);

    /* Setup right-click menu. */
    menu = gtk_menu_new();
    g_MainWindow.playRomItem = gtk_image_menu_item_new_with_label(tr("Play Rom"));
    g_MainWindow.playRombrowserImage =  gtk_image_new();
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(g_MainWindow.playRomItem), g_MainWindow.playRombrowserImage);
    g_signal_connect(g_MainWindow.playRomItem, "activate", G_CALLBACK(callback_play_rom), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), g_MainWindow.playRomItem);

    g_MainWindow.romPropertiesItem = gtk_image_menu_item_new_with_label(tr("Rom Properties"));
    g_MainWindow.propertiesRombrowserImage =  gtk_image_new();
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(g_MainWindow.romPropertiesItem), g_MainWindow.propertiesRombrowserImage);
    g_signal_connect(g_MainWindow.romPropertiesItem, "activate", G_CALLBACK(callback_rom_properties), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), g_MainWindow.romPropertiesItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    item = gtk_image_menu_item_new_with_label(tr("Refresh"));
    g_MainWindow.refreshRombrowserImage = gtk_image_new();
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), g_MainWindow.refreshRombrowserImage);
    g_signal_connect(item, "activate", G_CALLBACK(callback_call_rcs), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(tr("Configure Rombrowser"));
    g_signal_connect(item, "activate", G_CALLBACK(callback_configure_rombrowser), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    submenu = gtk_image_menu_item_new_with_label(tr("Configure Columns"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu), g_MainWindow.romHeaderMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

    gtk_widget_show_all(menu);

    g_signal_connect(g_MainWindow.romDisplay, "button-press-event", G_CALLBACK(callback_rombrowser_context), (gpointer)menu);
}

