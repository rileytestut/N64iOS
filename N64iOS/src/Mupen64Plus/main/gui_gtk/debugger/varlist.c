/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - varlist.c                                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 HyperHacker                                        *
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

#include <inttypes.h>

#include "varlist.h"

#include "../../translate.h"
#include "../../main.h"
//#include <ieee754.h>

enum { //Treeview columns
    Col_Name,       //Name of the variable.
    Col_Value,      //Current value.
    Col_Address,    //RAM address.
    Col_Type,       //Type/size.
    Col_Offset,     //For pointers, address=(*address)+offset
    Num_Columns
};

enum { //Variable types
    Type_int8,
    Type_int16,
    Type_int32,
    Type_int64,
    Type_uint8,
    Type_uint16,
    Type_uint32,
    Type_uint64,
    Type_hex8,
    Type_hex16,
    Type_hex32,
    Type_hex64,
    Type_float,
    Type_double,
    Num_Types,
    Type_pointer = 0x80 //flag, OR with one of above values
};

const char* type_name[Num_Types] = {
    "int8", "int16", "int32", "int64",
    "uint8", "uint16", "uint32", "uint64",
    "hex8", "hex16", "hex32", "hex64",
    "float", "double"
};

const char* col_name[Num_Columns] = {
    "Name", "Value", "Address", "Type", "Offset"
};

int iter_stamp;
GtkWidget *tree;
GtkTreeStore *store;
GtkTreeSelection *tree_selection;
GtkCellRenderer *cell_renderer[Num_Columns];

void value_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
void address_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
void offset_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
void type_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
static void on_add();
static void on_remove();
static void on_import();
static void on_cell_edit(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data);
static void on_auto_refresh_toggle(GtkToggleButton *togglebutton, gpointer user_data);
static void on_close();
static void add_value(char* name, uint32 address, int type, uint32 offset);
static void import_file(char* filename);

void update_varlist()
{
    gtk_widget_queue_draw(tree);
}

//]=-=-=-=-=-=-=-=-=-=-=-=[ Variable List Initialisation ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_varlist()
{
    int i;
    GtkTreeViewColumn *col[Num_Columns];
    GtkWidget *hbox, *vbox, *buAdd, *buRemove, *buAutoRefresh,
        *buImport;
    GValue cell_editable;

    varlist_auto_update = 0;
    iter_stamp = 0xD06FECE5; //unique stamp value (increments)
    
    //Create the window.
    winVarList = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW(winVarList), "Variables");
    gtk_window_set_default_size( GTK_WINDOW(winVarList), 300, 380);
    gtk_container_set_border_width( GTK_CONTAINER(winVarList), 2);
    
    //Create the containers.
    hbox = gtk_hbox_new(FALSE, 0); //hbox holds the treeview and hbox.
    vbox = gtk_vbox_new(FALSE, 0); //vbox holds the buttons at the right.
    //hbox, vbox, no xbox?
    
    //Create the treeview.
    store = gtk_tree_store_new(Num_Columns, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    
    //Create the cell renderers.
    memset(&cell_editable, 0, sizeof(cell_editable));
    g_value_init(&cell_editable, G_TYPE_BOOLEAN);
    g_value_set_boolean(&cell_editable, TRUE);
    
    for(i=0; i<Num_Columns; i++)
    {
        //cell_renderer[i] = (i == Col_Type) ? gtk_cell_renderer_combo_new() : gtk_cell_renderer_text_new();
        cell_renderer[i] = gtk_cell_renderer_text_new();
        g_object_set_property(G_OBJECT(cell_renderer[i]), "editable", &cell_editable);
    }
    
    //Enable multiple selection for remove button.
    tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_MULTIPLE);
    
    //Set up type combo box.
    /*g_value_set_boolean(&cell_editable, FALSE);
    g_object_set_property(G_OBJECT(cell_renderer[Col_Type]), "has-entry", &cell_editable);
    
    type_list = gtk_tree_store_new(1, G_TYPE_STRING);
    memset(&type_val, 0, sizeof(type_val));
    g_value_init(&type_val, G_TYPE_STRING);
    
    for(i=0; i<Num_Types; i++)
    {
        g_value_set_string(&type_val, type_name[i]);
        gtk_tree_store_append(type_list, &iter, NULL);
        gtk_tree_store_set_value(type_list, &iter, 0, &type_val);
    }
    
    memset(&model_val, 0, sizeof(model_val));
    g_value_init(&model_val, GTK_TYPE_TREE_MODEL);
    g_value_set_???(&model_val, &type_list); //what function goes here?
    g_object_set_property(G_OBJECT(cell_renderer[Col_Type]), "model", &model_val);
    printf("--------------1------------\n");
    
    memset(&model_col, 0, sizeof(model_col));
    g_value_init(&model_val, G_TYPE_INT);
    g_value_set_int(&model_col, 0);
    g_object_set_property(G_OBJECT(cell_renderer[Col_Type]), "text-column", &model_col);    
    printf("--------------2------------\n");*/
    
    
    //Create columns.
    for(i=0; i<Num_Columns; i++)
        col[i] = gtk_tree_view_column_new_with_attributes(col_name[i], cell_renderer[i], "text", i, NULL);
        
    gtk_tree_view_column_set_cell_data_func(col[Col_Value], cell_renderer[Col_Value], value_data_func, NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(col[Col_Address], cell_renderer[Col_Address], address_data_func, NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(col[Col_Type], cell_renderer[Col_Type], type_data_func, NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(col[Col_Offset], cell_renderer[Col_Offset], offset_data_func, NULL, NULL);
    
    //Add columns.
    for(i=0; i<Num_Columns; i++)
    {
        gtk_tree_view_column_set_resizable(col[i], TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col[i]);
    }
    
    //Add treeview to container.
    g_object_unref(G_OBJECT(store));
    gtk_box_pack_start(GTK_BOX(hbox), tree, TRUE, TRUE, 0);
    
    //Create buttons and add to container.
    buAdd = gtk_button_new_with_label("Add");
    gtk_box_pack_start(GTK_BOX(vbox), buAdd, FALSE, FALSE, 0);
    
    buRemove = gtk_button_new_with_label("Remove");
    gtk_box_pack_start(GTK_BOX(vbox), buRemove, FALSE, FALSE, 0);
    
    buImport = gtk_button_new_with_label("Import");
    gtk_box_pack_start(GTK_BOX(vbox), buImport, FALSE, FALSE, 0);
    
    buAutoRefresh = gtk_check_button_new_with_label("Auto Refresh");
    gtk_box_pack_start(GTK_BOX(vbox), buAutoRefresh, FALSE, FALSE, 0);
    
    //Add container to window.
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(winVarList), hbox);
    gtk_widget_show_all(winVarList);

    //=== Signal Connections ===========================/
    gtk_signal_connect(GTK_OBJECT(winVarList), "destroy", (GtkSignalFunc)on_close, NULL);
    gtk_signal_connect(GTK_OBJECT(buAdd), "clicked", (GtkSignalFunc)on_add, NULL);
    gtk_signal_connect(GTK_OBJECT(buRemove), "clicked", (GtkSignalFunc)on_remove, NULL);
    gtk_signal_connect(GTK_OBJECT(buAutoRefresh), "clicked", (GtkSignalFunc)on_auto_refresh_toggle, NULL);
    gtk_signal_connect(GTK_OBJECT(buImport), "clicked", (GtkSignalFunc)on_import, NULL);
    
    for(i=0; i<Num_Columns; i++)
        gtk_signal_connect(GTK_OBJECT(cell_renderer[i]), "edited", (GtkSignalFunc)on_cell_edit, (gpointer)(long)i);
    
    varlist_opened = 1;
    
    //test values
    //add_value("Lives", 0x8033B21D, Type_int8, 0);
    //add_value("Health", 0x8033B21E, Type_hex16, 0);
    //add_value("Pointer", 0x8033B200, Type_hex32 | Type_pointer, 4);
    //add_value("Float", 0x800F6A10, Type_float, 0);
}


//Draws the text for the 'value' column.
void value_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    GValue val_str, val_addr, val_type, val_offset;
    char str[512]; //just to be safe; 'double' values can be up to 307-digit numbers! possibly more!
    uint32 addr, type, offset = 0;
    uint64 magic;
    
    memset(&val_str, 0, sizeof(val_str)); //why do I have to do this? ._.
    memset(&val_addr, 0, sizeof(val_addr));
    memset(&val_type, 0, sizeof(val_type));
    memset(&val_offset, 0, sizeof(val_offset));
    g_value_init(&val_str, G_TYPE_STRING);
    
    //Get the address, offset and type
    gtk_tree_model_get_value(tree_model, iter, Col_Address, &val_addr);
    addr = g_value_get_int(&val_addr);
    
    gtk_tree_model_get_value(tree_model, iter, Col_Offset, &val_offset);
    offset = g_value_get_int(&val_offset);
    
    gtk_tree_model_get_value(tree_model, iter, Col_Type, &val_type);
    type = g_value_get_int(&val_type);
    
    //If it's a pointer, follow it.
    if(type & Type_pointer)
    {
        type &= ~Type_pointer;
        addr = read_memory_32_unaligned(addr) + offset;
        //not that it makes a lot of sense to read a pointer from an unaligned
        //address, but let them do it if they want...
    }
    
    if(get_memory_flags(addr) & MEM_FLAG_READABLE)
    {   
        //Read the variable and format it for display.
        switch(type)
        {
            case Type_int8:     sprintf(str, "%d", (char)read_memory_8(addr)); break;
            case Type_int16:    sprintf(str, "%d", (short)read_memory_16(addr)); break;
            case Type_int32:    sprintf(str, "%d", (int)read_memory_32_unaligned(addr)); break;
            case Type_int64:    sprintf(str, "%" PRId64, (long long int)read_memory_64_unaligned(addr)); break;
            
            case Type_uint8:    sprintf(str, "%u", (unsigned char)read_memory_8(addr)); break;
            case Type_uint16:   sprintf(str, "%u", (unsigned short)read_memory_16(addr)); break;
            case Type_uint32:   sprintf(str, "%u", (unsigned int)read_memory_32_unaligned(addr)); break;
            case Type_uint64:   sprintf(str, "%" PRIu64, (unsigned long long int)read_memory_64_unaligned(addr)); break;
            
            case Type_hex8:     sprintf(str, "%02X", read_memory_8(addr)); break;
            case Type_hex16:    sprintf(str, "%04X", read_memory_16(addr)); break;
            case Type_hex32:    sprintf(str, "%08X", read_memory_32_unaligned(addr)); break;
            case Type_hex64:    sprintf(str, "%08X %08X", read_memory_32_unaligned(addr), read_memory_32_unaligned(addr + 4)); break;
            
            case Type_float:
                addr = read_memory_32_unaligned(addr);
                sprintf(str, "%f", *((float*)&addr)); //magic conversion to IEEE754. I have no idea how this works.
            break;
            
            case Type_double:
                magic = read_memory_64_unaligned(addr);
                sprintf(str, "%f", *((double*)&magic));
            break;
            
            default:            sprintf(str, "??? (%X)", type); break;
        }
    }
    else strcpy(str, "-"); //unreadable
    
    //Set the cell text.
    g_value_set_string(&val_str, str);
    g_object_set_property(G_OBJECT(cell), "text", &val_str);
}


//Draws the text for the 'address' column.
void address_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    GValue val_str, val_addr;
    char str[9];
    
    memset(&val_str, 0, sizeof(val_str));
    memset(&val_addr, 0, sizeof(val_addr));
    g_value_init(&val_str, G_TYPE_STRING);
    
    gtk_tree_model_get_value(tree_model, iter, Col_Address, &val_addr);
    sprintf(str, "%08X", g_value_get_int(&val_addr));
    
    g_value_set_string(&val_str, str);
    g_object_set_property(G_OBJECT(cell), "text", &val_str);
}


//Draws the text for the 'offset' column.
void offset_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    GValue val_str, val_offs;
    char str[9];
    
    memset(&val_str, 0, sizeof(val_str));
    memset(&val_offs, 0, sizeof(val_offs));
    g_value_init(&val_str, G_TYPE_STRING);
    
    gtk_tree_model_get_value(tree_model, iter, Col_Offset, &val_offs);
    sprintf(str, "%X", g_value_get_int(&val_offs));
    
    g_value_set_string(&val_str, str);
    g_object_set_property(G_OBJECT(cell), "text", &val_str);
}


//Draws the text for the 'type' column.
void type_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    GValue val_str, val_type;
    char str[32];
    int type;
    
    memset(&val_str, 0, sizeof(val_str));
    memset(&val_type, 0, sizeof(val_type));
    g_value_init(&val_str, G_TYPE_STRING);
    
    gtk_tree_model_get_value(tree_model, iter, Col_Type, &val_type);
    type = g_value_get_int(&val_type);
    
    strncpy(str, type_name[type & ~Type_pointer], sizeof(str));
    if(type & Type_pointer) strcat(str, "*");
    
    g_value_set_string(&val_str, str);
    g_object_set_property(G_OBJECT(cell), "text", &val_str);
}


//'Add' button handler.
static void on_add()
{
    uint32 addr, numbytes;
    int type;
    
    numbytes = GetMemEditSelectionRange(&addr, 1);
    switch(numbytes)
    {
        case 0:
            addr = 0x80000000;
            type = Type_int32;
        break;
        
        case 1: type = Type_int8; break;
        case 2: type = Type_int16; break;
        case 3: type = Type_hex32; break;
        case 4: type = Type_int32; break;
        default: type = Type_hex64; break;
    }
    add_value("No name", addr, type, 0); //todo: prompt for info
}


//'Remove' button handler.
static void on_remove()
{
    GList *selected, *element;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    selected = gtk_tree_selection_get_selected_rows(tree_selection, &model);
    element = g_list_last(selected); //iterate backward so we don't miss any
    while(element != NULL)
    {
        if(!gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)element->data)) break;
        gtk_tree_store_remove(store, &iter);
        element = g_list_previous(element);
    }
    
    g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected);
}


//'Import' button handler.
static void on_import()
{
    GtkWidget *dialog;
    char* filename;

    //Display file selector
    //This is currently broken so I've hardcoded a path.
    //import_file("/home/hyperhacker/Desktop/mupenvars.txt");
    //return;
    
    dialog = gtk_file_chooser_dialog_new("Import Variable List", GTK_WINDOW(winVarList),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    //dialog = gtk_file_chooser_dialog_new(NULL, NULL, GTK_FILE_CHOOSER_ACTION_OPEN, NULL);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        import_file(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


//Cell edit handler.
//Todo: why is this getting called on double-click when the text box hasn't
//popped up yet?
static void on_cell_edit(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data)
{
    int i, j;
    GtkTreeIter iter;
    GValue newval, val_addr, val_type, val_offset;
    uint32 addr, type, value, offset;
    uint64 value64;
    float valuef;
    double valued;
    char formatted[18]; //for reading hex64 values
    
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
    memset(&newval, 0, sizeof(newval));
    memset(&val_addr, 0, sizeof(val_addr));
    memset(&val_type, 0, sizeof(val_type));
    memset(&val_offset, 0, sizeof(val_offset));
    
    switch((long)user_data) //column number
    {
        case Col_Name:
            g_value_init(&newval, G_TYPE_STRING);
            g_value_set_string(&newval, new_text);
        break;
        
        case Col_Value:
            //Get address
            gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, Col_Address, &val_addr);
            addr = g_value_get_int(&val_addr);
            
            //Get type
            gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, Col_Type, &val_type);
            type = g_value_get_int(&val_type);
            
            //Get offset
            gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, Col_Offset, &val_offset);
            offset = g_value_get_int(&val_offset);
            
            //If pointer, follow it.
            if(type & Type_pointer)
            {
                type &= ~Type_pointer;
                addr = read_memory_32(addr) + offset;
            }
            
            //Get new value and write.
            //Todo: should not write if value is invalid, or strip any non-
            //numeric characters from value, to avoid the annoying bug where
            //you make a typo and it gets set to zero.
            //If we just copy the current value into the variables sscanf() is
            //supposed to set, it won't change them when the format string is
            //invalid. Not sure what happens if it's too long.
            switch(type)
            {
                case Type_int8:
                    sscanf(new_text, "%d", &value);
                    write_memory_8(addr, value);
                break;
                
                case Type_int16:
                    sscanf(new_text, "%d", &value);
                    write_memory_16(addr, value);
                break;
                
                case Type_int32:
                    sscanf(new_text, "%d", &value);
                    write_memory_32(addr, value);
                break;
                
                case Type_int64:
                    sscanf(new_text, "%" PRId64, &value64);
                    write_memory_64(addr, value64);
                break;
                
                case Type_uint8:
                    sscanf(new_text, "%u", &value);
                    write_memory_8(addr, value);
                break;
                
                case Type_uint16:
                    sscanf(new_text, "%u", &value);
                    write_memory_16(addr, value);
                break;
                
                case Type_uint32:
                    sscanf(new_text, "%u", &value);
                    write_memory_32(addr, value);
                break;
                
                case Type_uint64:
                    sscanf(new_text, "%" PRIu64, &value64);
                    write_memory_64(addr, value64);
                break;
                
                case Type_hex8:
                    sscanf(new_text, "%X", &value);
                    write_memory_8(addr, value);
                break;
                
                case Type_hex16:
                    sscanf(new_text, "%X", &value);
                    write_memory_16(addr, value);
                break;
                
                case Type_hex32:
                    sscanf(new_text, "%X", &value);
                    write_memory_32(addr, value);
                break;
                
                case Type_hex64:
                    //Copy new text without spaces so it can be parsed correctly.
                    j = 0;
                    for(i=0; new_text[i]; i++)
                    {
                        if(new_text[i] == ' ') continue;
                        formatted[j] = new_text[i];
                        j++;
                    }
                    formatted[j] = '\0';
                    sscanf(formatted, "%" PRIX64, &value64);
                    write_memory_64(addr, value64);
                break;
                
                case Type_float:
                    //todo: the value needs to be converted to IEEE 754 somehow.
                    sscanf(new_text, "%f", &valuef);
                    write_memory_32(addr, (int)value);
                break;
                
                case Type_double:
                    sscanf(new_text, "%lf", &valued);
                    write_memory_64(addr, (uint64)value);
                break;
                
                default:
                    printf("on_cell_edit(): unknown type %d in \"%s\", col %d\n", type, path, (int)(long)user_data);
                    return;
                break;
            }
        break;
        
        case Col_Address:
            g_value_init(&newval, G_TYPE_INT);
            sscanf(new_text, "%X", &addr);
            g_value_set_int(&newval, addr);
        break;
        
        case Col_Type:
            //todo - this should actually be a dropdown list, not editable,
            //if I had any idea how to do that.
            if(strlen(new_text) > 2) return; //so "float" doesn't get parsed as 0xF
            addr = 0x7F;
            g_value_init(&newval, G_TYPE_INT);
            sscanf(new_text, "%X", &addr);
            if((addr & 0x7F) >= Num_Types) return;
            g_value_set_int(&newval, addr);
        break;
        
        case Col_Offset:
            g_value_init(&newval, G_TYPE_INT);
            sscanf(new_text, "%X", &addr);
            g_value_set_int(&newval, addr);
        break;
    }
    
    if((long)user_data != Col_Value)
        gtk_tree_store_set_value(store, &iter, (int)(long)user_data, &newval);
}


//Auto-refresh checkbox handler.
static void on_auto_refresh_toggle(GtkToggleButton *togglebutton, gpointer user_data)
{
    varlist_auto_update = gtk_toggle_button_get_active(togglebutton) ? 1 : 0;
}


//Called when window is closed.
static void on_close()
{
    varlist_opened = 0;
}


//Add a value to the variable list.
static void add_value(char* name, uint32 address, int type, uint32 offset)
{
    GValue nameval, addrval, typeval, offsetval;
    GtkTreeIter iter;
    
    iter.stamp = iter_stamp;
    iter_stamp++;

    //We store the variable info in the treeview itself.
    //Believe it or not, it's easier that way.  
    memset(&nameval, 0, sizeof(nameval));
    memset(&addrval, 0, sizeof(addrval));
    memset(&typeval, 0, sizeof(typeval));
    memset(&offsetval, 0, sizeof(offsetval));
    
    g_value_init(&nameval, G_TYPE_STRING);
    g_value_init(&addrval, G_TYPE_INT);
    g_value_init(&typeval, G_TYPE_INT);
    g_value_init(&offsetval, G_TYPE_INT);
    
    g_value_set_string(&nameval, name);
    g_value_set_int(&addrval, address);
    g_value_set_int(&typeval, type);
    g_value_set_int(&offsetval, offset);
    
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set_value(store, &iter, Col_Name, &nameval);
    //We don't bother to set a value for the "value" column, it gets
    //filled in on refresh.
    gtk_tree_store_set_value(store, &iter, Col_Address, &addrval);
    gtk_tree_store_set_value(store, &iter, Col_Type, &typeval);
    gtk_tree_store_set_value(store, &iter, Col_Offset, &offsetval);
}


//Imports a variable list from a file.
static void import_file(char* filename)
{
    FILE *file;
    char line[1024];
    char *data, *next;
    int type = 0;
    uint32 address = 0x80000000, offset = 0;
    
    file = fopen(filename, "rt");
    if(!file)
    {
        //todo: show error message
        sprintf(line, "Cannot open file \"%s\"\n", filename);
        error_message(tr(line));
        return;
    }
    
    while(!feof(file))
    {
        fgets(line, sizeof(line), file);
        if((line[0] == '\n') || (line[0] == ';') || (line[0] == '#') || (line[0] == '\0')) continue;
        
        //Get type.
        //This is fairly lazy checking that doesn't check for invalid types.
        next = strchr(line, ' ');
        if(!next) continue;
        next[0] = '\0';
        if((line[0] == 'f') || (line[0] == 'F')) type = Type_float;
        else if((line[0] == 'd') || (line[0] == 'D')) type = Type_double;
        else
        {
            switch(line[1])
            {
                case '8': type = Type_int8; break;
                case '1': type = Type_int16; break;
                case '3': type = Type_int32; break;
                case '6': type = Type_int64; break;
            }
            
            if((line[0] == 'u') || (line[0] == 'U')) type += 4; //unsigned
            else if((line[0] == 'h') || (line[0] == 'H')) type += 8; //hex
        }
        
        if(next[-1] == '*') //pointer
            type |= Type_pointer;
            
        
        //Get address.
        data = &next[1];
        next = strchr(data, ' ');
        if(!next) continue;
        next[0] = '\0';
        sscanf(data, "%X", &address);
        
        
        //Get offset if pointer.
        if(type & Type_pointer)
        {
            data = &next[1];
            next = strchr(data, ' ');
            if(!next) continue;
            next[0] = '\0';
            sscanf(data, "%X", &offset);
        }
        else offset = 0;
        
        
        //Strip line break.
        data = &next[1];
        next = strchr(data, '\n');
        if(next) (*next) = '\0';
        
        //Add to list.
        add_value(data, address, type, offset);
    }
    
    fclose(file);
}

