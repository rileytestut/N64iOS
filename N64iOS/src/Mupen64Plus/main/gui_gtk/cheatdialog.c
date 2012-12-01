/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main_gtk.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 ebenblues                                          *
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

#include "main_gtk.h"

#include "main_gtk.h"
#include "cheatdialog.h"
#include "../cheat.h"
#include "../translate.h"
#include "../util.h"

#include "../gui.h"

/** globals **/
static GtkWidget *g_CheatView = NULL;

// column numbering for cheat code list store
enum 
{
    ADDRESS_COLUMN,
    VALUE_COLUMN,
    CHEAT_CODE_COLUMN,
    NUM_COLUMNS
};

/** private functions **/
// user clicked ok button
static void cb_response(GtkWidget *widget, gint response, gpointer data)
{
    cheat_write_config();
    gtk_widget_destroy(widget);
}

// user clicked button to add a new cheat code
static void cb_addCode(GtkWidget *button, GtkTreeView *treeview)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection;

    cheat_t *cheat;
    cheat_code_t *cheatcode;

    // first, get cheat that these codes belong to
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_CheatView));
    gtk_tree_selection_get_selected(selection, &model, &iter);

    gtk_tree_model_get(model, &iter, 1, &cheat, -1);

    // create new cheat code
    cheatcode = cheat_new_cheat_code(cheat);

    // add new cheat code row to list
    model = gtk_tree_view_get_model(treeview);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       ADDRESS_COLUMN, cheatcode->address,
                       VALUE_COLUMN, cheatcode->value,
                       CHEAT_CODE_COLUMN, cheatcode,
                       -1);

    // select new row
    selection = gtk_tree_view_get_selection(treeview);
    gtk_tree_selection_select_iter(selection, &iter);
}

// user clicked button to remove a cheat code
static void cb_removeCode(GtkWidget *button, GtkTreeView *treeview)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection;

    cheat_t *cheat;
    cheat_code_t *cheatcode;

    // first, get cheat that these codes belong to
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_CheatView));
    gtk_tree_selection_get_selected(selection, &model, &iter);

    gtk_tree_model_get(model, &iter, 1, &cheat, -1);

    // get cheat code pointer, then delete row from tree model
    selection = gtk_tree_view_get_selection(treeview);
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        {
        gtk_tree_model_get(model, &iter, CHEAT_CODE_COLUMN, &cheatcode, -1);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

        // delete cheat code structure
        cheat_delete_cheat_code(cheat, cheatcode);
        }

    //If there are codes left, select the first one.
    if(gtk_tree_model_get_iter_first(model, &iter))
        gtk_tree_selection_select_iter(selection, &iter);
}

// user edited cheat code (address or value)
static void cb_editCode(GtkCellRenderer *cell, gchar *path_string, gchar *new_text, GtkTreeView *view)
{
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    guint col_num;

    cheat_code_t *cheatcode;

    // get column and row information
    gtk_tree_model_get_iter_from_string(model, &iter, path_string);
    col_num = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(cell), "col_num"));

    // get pointer to cheatcode struct
    gtk_tree_model_get(model, &iter, CHEAT_CODE_COLUMN, &cheatcode, -1);

    // user edited address cell
    if(col_num == ADDRESS_COLUMN)
        {
        cheatcode->address = strtoumax(new_text, NULL, 16);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           ADDRESS_COLUMN, cheatcode->address,
                           -1);
        }
    // user edited value cell
    else
        {
        cheatcode->value = strtoumax(new_text, NULL, 16);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           VALUE_COLUMN, cheatcode->value,
                           -1);
        }
}


// returns tree view of all user cheats
static GtkWidget *create_cheat_treeview(void)
{
    GtkWidget *view;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkTreeStore *model;
    GtkTreeIter romIter,
                cheatIter;

    list_node_t *romnode,
                *cheatnode;
    rom_cheats_t *romcheat;
    cheat_t *cheat;

    // model stores either a rom name or cheat name, and a pointer to either the rom_cheat_t or cheat_t struct
    model = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

    // populate model
    list_foreach(g_Cheats, romnode)
        {
        romcheat = (rom_cheats_t *)romnode->data;

        // add rom name to tree
        gtk_tree_store_append(model, &romIter, NULL);
        gtk_tree_store_set(model, &romIter,
                           0, romcheat->rom_name,
                           1, romcheat,
                           -1);

        // add all cheats for this rom as children in the tree
        list_foreach(romcheat->cheats, cheatnode)
            {
            cheat = (cheat_t *)cheatnode->data;

            gtk_tree_store_append(model, &cheatIter, &romIter);
            gtk_tree_store_set(model, &cheatIter, 0, cheat->name, 1, cheat, -1);
            }
        }

    // create tree view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

    // allows model memory to be freed when tree view is destroyed.
    g_object_unref(model);

    // only add the string column to the view, the struct pointer is hidden data
    rend = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(col, rend, TRUE);
//    gtk_tree_view_column_set_resizable (col, FALSE);
    gtk_tree_view_column_add_attribute(col, rend,  "text", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    return view;
}

// Adds new rom to cheat structures and tree view, and selects it so user can edit it
static void cb_newRom(GtkButton *button, GtkTreeView *view)
{
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    GtkTreeSelection *selection;

    rom_cheats_t *romcheat = cheat_new_rom();
    romcheat->rom_name = strdup(tr("New Rom"));

    // add new rom to tree model
    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                       0, romcheat->rom_name,
                       1, romcheat,
                       -1);

    // select new rom
    selection = gtk_tree_view_get_selection(view);
    gtk_tree_selection_select_iter(selection, &iter);
}

// Adds new cheat to cheat structures and tree view, and selects it so user can edit it
static void cb_newCheat(GtkButton *button, GtkTreeView *view)
{
    GtkTreeModel *model;
    GtkTreeIter iter, selectedIter, parentIter, *romIter;
    GtkTreeSelection *selection;
    GtkTreePath *path;

    rom_cheats_t *romcheat;
    cheat_t *cheat;

    selection = gtk_tree_view_get_selection(view);

    // something should always be selected, but just in case...
    if(gtk_tree_selection_get_selected(selection, &model, &selectedIter))
        {

        // get pointer to rom iter
        if(gtk_tree_model_iter_parent(model, &parentIter, &selectedIter))
            romIter = &parentIter;
        else
            romIter = &selectedIter;

        // grab pointer to rom cheat data out of model
        gtk_tree_model_get(model, romIter, 1, &romcheat, -1);

        // create new cheat
        cheat = cheat_new_cheat(romcheat);
        cheat->name = strdup(tr("New Cheat"));

        // add new cheat to the tree model
        gtk_tree_store_append(GTK_TREE_STORE(model), &iter, romIter);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                           0, cheat->name,
                           1, cheat,
                           -1);

        // expand rom row if it isn't already
        path = gtk_tree_model_get_path(model, romIter);
        gtk_tree_view_expand_row(view, path, FALSE);
        gtk_tree_path_free(path);

        // select new cheat
        gtk_tree_selection_select_iter(selection, &iter);
        }
}

// Remove currently selected rom/cheat from the tree
static void cb_delete(GtkButton *button, GtkTreeView *view)
{
    GtkTreeModel *model;
    GtkTreeIter iter, parentIter;
    GtkTreeSelection *selection;

    rom_cheats_t *romcheat = NULL;
    cheat_t *cheat = NULL;

    selection = gtk_tree_view_get_selection(view);

    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        {
        // check if we're deleting a rom or cheat
        if(gtk_tree_model_iter_parent(model, &parentIter, &iter))
            {
            // selected item is a cheat
            gtk_tree_model_get(model, &parentIter, 1, &romcheat, -1);
            gtk_tree_model_get(model, &iter, 1, &cheat, -1);

            if(gui_message(GUI_MESSAGE_CONFIRM,
                           tr("Are you sure you want to delete cheat \"%s\"?"),
                           cheat->name))
                {
                gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
                cheat_delete_cheat(romcheat, cheat);

                // set selection to parent rom
                gtk_tree_selection_select_iter(selection, &parentIter);
                }
            }
        else
            {
            // selected item is a rom
            gtk_tree_model_get(model, &iter, 1, &romcheat, -1);

            if(gui_message(GUI_MESSAGE_CONFIRM,
                           tr("Are you sure you want to delete rom \"%s\" and all of its cheats?"),
                           romcheat->rom_name))
                {
                gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
                cheat_delete_rom(romcheat);

                //If there any roms are left, set selection to first rom.
                if(gtk_tree_model_get_iter_first(model, &iter))
                    gtk_tree_selection_select_iter(selection, &iter);
                }
            }
       }
}

// Updates tree model and rom_cheats_t structure with new rom name
static void cb_romNameChanged(GtkWidget *textbox, GtkTreeSelection *selection)
{
    gchar *text = gtk_editable_get_chars(GTK_EDITABLE(textbox), 0, -1);
    GtkTreeIter iter;
    GtkTreeModel *model;

    rom_cheats_t *romcheat;

    // if we're here, something should be selected, but just in case...
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        {
        gtk_tree_model_get(model, &iter, 1, &romcheat, -1);

        // change rom name in rom_cheats_t struct
        if(romcheat->rom_name)
            free(romcheat->rom_name);
        romcheat->rom_name = strdup(text);

        // change rom name in tree model
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, romcheat->rom_name, -1);
        }

    g_free(text);
}

// Updates rom_cheats_t structure with new rom crc
static void cb_crcChanged(GtkWidget *textbox, unsigned int *rom_crc)
{
    gchar *text = gtk_editable_get_chars(GTK_EDITABLE(textbox), 0, -1);
    *rom_crc = strtoumax(text, NULL, 16);
    g_free(text);
}

// Returns user form to edit rom info.
static GtkWidget *edit_rom_info_widget(rom_cheats_t *romcheat, GtkTreeSelection *selection)
{
    int i;
    char crc[9];

    GtkWidget *label;
    GtkWidget *table;
    GtkWidget *textbox;
    GtkWidget *mainvbox;

    mainvbox = gtk_vbox_new( 0, FALSE );
    gtk_container_set_border_width( GTK_CONTAINER(mainvbox), 10);

    table = gtk_table_new( 2, 3, FALSE );
    gtk_table_set_col_spacings( GTK_TABLE(table), 5 );
    gtk_table_set_row_spacings( GTK_TABLE(table), 5 );
    gtk_box_pack_start( GTK_BOX(mainvbox), table, FALSE, FALSE, 0 );

    // Rom name
    label = gtk_label_new(tr("Name:"));
    gtk_misc_set_alignment( GTK_MISC(label), 1, 0 );
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

    textbox = gtk_entry_new();
    gtk_widget_set_size_request(textbox, 170, -1);
    if(romcheat->rom_name)
        gtk_editable_insert_text(GTK_EDITABLE(textbox), romcheat->rom_name, strlen(romcheat->rom_name), &i);

    // connect signal such that as the user changes the Rom name in the textbox, it's updated in the model
    g_signal_connect(textbox, "changed", G_CALLBACK(cb_romNameChanged), selection);
    gtk_table_attach_defaults(GTK_TABLE(table), textbox, 1, 3, 0, 1);

    // Rom CRC
    label = gtk_label_new(tr("CRC:"));
    gtk_misc_set_alignment( GTK_MISC(label), 1, 0 );
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    // CRC1 textbox
    textbox = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(textbox), 8);
    gtk_widget_set_size_request(textbox, 85, -1);
    snprintf(crc, 9, "%.8x", romcheat->crc1);
    gtk_entry_set_text(GTK_ENTRY(textbox), crc);

    // connect signal such that as the user changes the CRC value in the textbox, it's updated in the rom_cheats_t struct
    g_signal_connect(textbox, "changed", G_CALLBACK(cb_crcChanged), &romcheat->crc1);
    gtk_table_attach_defaults(GTK_TABLE(table), textbox, 1, 2, 1, 2);

    // CRC2 textbox
    textbox = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(textbox), 8);
    gtk_widget_set_size_request(textbox, 85, -1);
    snprintf(crc, 9, "%.8x", romcheat->crc2);
    gtk_entry_set_text(GTK_ENTRY(textbox), crc);

    // connect signal such that as the user changes the CRC value in the textbox, it's updated in the rom_cheats_t struct
    g_signal_connect(textbox, "changed", G_CALLBACK(cb_crcChanged), &romcheat->crc2);
    gtk_table_attach_defaults(GTK_TABLE(table), textbox, 2, 3, 1, 2);

    return mainvbox;
}

// Updates tree model and cheat_t structure with new cheat name
static void cb_cheatNameChanged(GtkWidget *textbox, GtkTreeSelection *selection)
{
    gchar *text = gtk_editable_get_chars(GTK_EDITABLE(textbox), 0, -1);
    GtkTreeIter iter;
    GtkTreeModel *model;

    cheat_t *cheat;

    // if we're here, something should be selected, but just in case...
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        {
        gtk_tree_model_get(model, &iter, 1, &cheat, -1);

        // change cheat name in cheat_t struct
        if(cheat->name)
            free(cheat->name);
        cheat->name = strdup(text);

        // change rom name in tree model
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, cheat->name, -1);
        }

    g_free(text);
}

// user disabled cheat
static void cb_cheatDisabled(GtkWidget *button, cheat_t *cheat)
{
    cheat->always_enabled = cheat->enabled = 0;
}

// user enabled cheat for this session only
static void cb_cheatEnabled(GtkWidget *button, cheat_t *cheat)
{
    cheat->enabled = 1;
    cheat->always_enabled = 0;
}

// user enabled cheat always
static void cb_cheatEnabledAlways(GtkWidget *button, cheat_t *cheat)
{
    cheat->enabled = 0;
    cheat->always_enabled = 1;
}

// function to display cheat codes in hex format
static void cb_cheat_data_func(GtkTreeViewColumn *col,
                               GtkCellRenderer *cell,
                               GtkTreeModel *tree_model,
                               GtkTreeIter *iter,
                               gpointer column)
{
    unsigned int val;
    char buf[20];

    // get cell value from model
    gtk_tree_model_get(tree_model, iter, column, &val, -1);
    if(column == ADDRESS_COLUMN)
        snprintf(buf, 20, "%.8x", val);
    else
        snprintf(buf, 20, "%.4x", val);

    // set cell text to value
    g_object_set(G_OBJECT(cell), "text", buf, NULL);
}

void gtk_tree_view_column_set_width(GtkTreeViewColumn *treeviewcolumn, gint width)
{
    treeviewcolumn->resized_width = width;
    treeviewcolumn->use_resized_width = TRUE;
    g_object_set(G_OBJECT(treeviewcolumn), "expand", FALSE, NULL);
}

// Returns tree view of all cheat codes for the given cheat
static GtkWidget *create_cheat_code_treeview(cheat_t *cheat)
{
    GtkWidget *view;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkListStore *model;
    GtkTreeIter iter;

    list_node_t *node;
    cheat_code_t *cheatcode;

    model = gtk_list_store_new(NUM_COLUMNS,
                               G_TYPE_UINT, // address column
                               G_TYPE_UINT, // value column
                               G_TYPE_POINTER); // hidden pointer to cheat_code_t struct

    // populate model with cheat codes
    list_foreach(cheat->cheat_codes, node)
        {
        cheatcode = (cheat_code_t *)node->data;

        // add cheat code to list model
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
                           ADDRESS_COLUMN, cheatcode->address,
                           VALUE_COLUMN, cheatcode->value,
                           CHEAT_CODE_COLUMN, cheatcode,
                           -1);
        }

    // create tree view
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

    // allows list memory to be freed when tree view is destroyed
    g_object_unref(model);

    // address column
    rend = gtk_cell_renderer_text_new();
    // attach column identifier to this renderer
    g_object_set_data(G_OBJECT(rend), "col_num", GUINT_TO_POINTER(ADDRESS_COLUMN));
    // allow user to edit cells
    g_object_set(rend, "editable", TRUE, NULL);
    g_signal_connect(rend, "edited", G_CALLBACK(cb_editCode), GTK_TREE_VIEW(view));
    col = gtk_tree_view_column_new_with_attributes(tr("Address"), rend,
                                                   "text", ADDRESS_COLUMN,
                                                   NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend, cb_cheat_data_func,
                                            (gpointer)ADDRESS_COLUMN, NULL);
    gtk_tree_view_column_set_width (col, 70);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    // value column
    rend = gtk_cell_renderer_text_new();
    // attach column identifier to this renderer
    g_object_set_data(G_OBJECT(rend), "col_num", GUINT_TO_POINTER(VALUE_COLUMN));
    // allow user to edit cells
    g_object_set(rend, "editable", TRUE, NULL);
    g_signal_connect(rend, "edited", G_CALLBACK(cb_editCode), GTK_TREE_VIEW(view));
    col = gtk_tree_view_column_new_with_attributes(tr("Value"), rend,
                                                   "text", VALUE_COLUMN,
                                                   NULL);
    gtk_tree_view_column_set_cell_data_func(col, rend, cb_cheat_data_func,
                                            (gpointer)VALUE_COLUMN, NULL);
    gtk_tree_view_column_set_width (col, 40);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    return view;
}

// Returns user form to edit a cheat (name, codes)
static GtkWidget *edit_cheat_widget(cheat_t *cheat, GtkTreeSelection *selection)
{
    int i;

    GtkWidget *table;
    GtkWidget *hbox, *buttonvbox, *mainvbox;
    GtkWidget *label;
    GtkWidget *textbox;
    GtkWidget *button;
    GtkWidget *code_treeview;
    GtkWidget *viewport;

    mainvbox = gtk_vbox_new( 0, FALSE );
    gtk_box_set_spacing(GTK_BOX(mainvbox), 5);
    gtk_container_set_border_width( GTK_CONTAINER(mainvbox), 10);

    table = gtk_table_new(4, 2, FALSE);
    gtk_table_set_col_spacings( GTK_TABLE(table), 5 );
    gtk_box_pack_start( GTK_BOX(mainvbox), table, FALSE, FALSE, 0 );

    // Cheat name
    label = gtk_label_new(tr("Name:"));
    gtk_misc_set_alignment( GTK_MISC(label), 1, 0 );
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

    textbox = gtk_entry_new();
    gtk_widget_set_size_request(textbox, 170, -1);
    if(cheat->name)
       gtk_editable_insert_text(GTK_EDITABLE(textbox), cheat->name, strlen(cheat->name), &i);

    // connect signal such that as the user changes the cheat name in the textbox, it's updated in the model
    g_signal_connect(textbox, "changed", G_CALLBACK(cb_cheatNameChanged), selection);
    gtk_table_attach_defaults(GTK_TABLE(table), textbox, 1, 2, 0, 1);

    // Cheat enable
    button = gtk_radio_button_new_with_label(NULL, tr("Disabled"));
    g_signal_connect(button, "toggled", G_CALLBACK(cb_cheatDisabled), cheat);
    gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 1, 2);

    button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), tr("Enabled  (Session)"));
    g_signal_connect(button, "toggled", G_CALLBACK(cb_cheatEnabled), cheat);
    if(cheat->enabled)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 2, 3);

    button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), tr("Enabled (Always)"));
    g_signal_connect(button, "toggled", G_CALLBACK(cb_cheatEnabledAlways), cheat);
    if(cheat->always_enabled)
       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 3, 4);

    // create tree view of cheat codes (address/value pairs)
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(hbox), 10);

    viewport = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(viewport), GTK_SHADOW_IN );
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(viewport),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(hbox), viewport, TRUE, TRUE, 0);

    code_treeview = create_cheat_code_treeview(cheat);
    gtk_container_add(GTK_CONTAINER(viewport), code_treeview);

    // create column of add/remove buttons next to cheat codes
    buttonvbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), buttonvbox, FALSE, FALSE, 0);

    // add cheat code button
    button = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_addCode), code_treeview);
    gtk_box_pack_start(GTK_BOX(buttonvbox), button, FALSE, FALSE, 5);

    // remove cheat code button
    button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_removeCode), code_treeview);
    gtk_box_pack_start(GTK_BOX(buttonvbox), button, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(mainvbox), hbox, TRUE, TRUE, 0);

    return mainvbox;
}

// Update contents of the detail frame with the details of the selected rom or cheat
static void cb_updateFrame(GtkTreeSelection *selection, GtkWidget *frame)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget *frame_contents;
    GtkWidget *label;

    rom_cheats_t *romcheat;
    cheat_t *cheat;

    // empty current frame contents
    frame_contents = gtk_bin_get_child(GTK_BIN(frame));
    if(frame_contents)
        gtk_widget_destroy(frame_contents);

    //If there are no cheats, put a label in the frame saying so.
    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
        {
        if(!gtk_tree_model_get_iter_first(model, &iter))
            { 
            gtk_frame_set_label(GTK_FRAME(frame), NULL);
            label = gtk_label_new(tr("No cheats found. Click\n \"New Rom\" to begin."));
            frame_contents = label;
            gtk_container_add(GTK_CONTAINER(frame), frame_contents);
            if(GTK_WIDGET_VISIBLE(frame))
                gtk_widget_show_all(frame_contents);
            return;
            }
        else
            {
            gtk_tree_selection_select_iter(selection, &iter); 
            return; //Don't continue after recursive calling.
            }
        }
 
    // depth of 0 means the selected item is a rom
    if(gtk_tree_store_iter_depth(GTK_TREE_STORE(model), &iter) == 0)
        {
        gtk_frame_set_label(GTK_FRAME(frame), "Edit Rom");
        // grab pointer to rom cheat data out of model
        gtk_tree_model_get(model, &iter, 1, &romcheat, -1);
        frame_contents = edit_rom_info_widget(romcheat, selection);
        }
    // depth of 1 means the selected item is a cheat
    else if(gtk_tree_store_iter_depth(GTK_TREE_STORE(model), &iter) == 1)
        {
        gtk_frame_set_label(GTK_FRAME(frame), "Edit Cheat");
        // grab pointer to cheat data out of model
        gtk_tree_model_get(model, &iter, 1, &cheat, -1);
        frame_contents = edit_cheat_widget(cheat, selection);
        }

   gtk_container_add(GTK_CONTAINER(frame), frame_contents);

    // if we're being called to update the frame, display the updated frame contents
    if(GTK_WIDGET_VISIBLE(frame))
       gtk_widget_show_all(frame_contents);
}

/** public functions **/

// Create and display cheat dialog
void cb_cheatDialog(GtkWidget *widget)
{
    // Local Variables
    GtkWidget *dialog;
    GtkWidget *frame;
    GtkWidget *hbox, *vbox;
    GtkWidget *viewport;
    GtkWidget *button;
    GtkWidget *align;

    GtkTreeSelection *selection;

    // Create Dialog Window
    dialog = gtk_dialog_new_with_buttons(tr("Configure Cheats"),
                                         GTK_WINDOW(g_MainWindow.window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    gtk_widget_set_size_request(dialog, 510, -1);
    gtk_container_set_border_width( GTK_CONTAINER(dialog), 10 );

    g_CheatView = create_cheat_treeview();

    g_signal_connect(dialog, "response",  G_CALLBACK(cb_response), g_CheatView);

    // create scrolled window for tree view
    viewport = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(viewport), GTK_SHADOW_IN );
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(viewport),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(viewport, 200, 400);
    gtk_container_add(GTK_CONTAINER(viewport), g_CheatView);

    // add cheat tree view within viewport to left side of dialog box
    hbox = gtk_hbox_new(FALSE, 10);
    align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
    gtk_alignment_set_padding( GTK_ALIGNMENT(align), 0, 10, 0, 0 );
    gtk_container_add( GTK_CONTAINER(align), hbox );
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), viewport, FALSE, FALSE, 0);

    // right side of dialog
    vbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 10);
    
    // row of add rom, add cheat, remove buttons
    hbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

    button = gtk_button_new_with_mnemonic(tr("_New Rom"));
    g_signal_connect(button, "clicked", G_CALLBACK(cb_newRom), g_CheatView);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_mnemonic(tr("New _Cheat"));
    g_signal_connect(button, "clicked", G_CALLBACK(cb_newCheat), g_CheatView);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_mnemonic(tr("_Delete"));
    g_signal_connect(button, "clicked", G_CALLBACK(cb_delete), g_CheatView);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    // frame containing detail of selected item in tree view (allows user to edit)
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    // setup callback such that whenever tree view selection is changed, frame gets updated
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_CheatView));
    g_signal_connect(selection, "changed", G_CALLBACK(cb_updateFrame), frame);

    // manually update the frame the first time
    cb_updateFrame(selection, frame);

    gtk_widget_show_all(dialog);
}

