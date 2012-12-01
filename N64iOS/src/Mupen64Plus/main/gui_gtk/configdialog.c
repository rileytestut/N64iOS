/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - configdialog.c                                          *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Blight                                             *
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
 
/* configdialog.c - Handles the configuration dialog */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "main_gtk.h"

#include <SDL.h>

#include "configdialog.h"
#include "main_gtk.h"
#include "rombrowser.h"
#include "icontheme.h"

#include "../main.h"
#include "../romcache.h"
#include "../config.h"
#include "../translate.h"
#include "../util.h"
#include "../winlnxdefs.h"
#include "../plugin.h"
#include "../savestates.h"

#include "../../opengl/osd.h"

#include "../../r4300/r4300.h"

/* Globals. */
SConfigDialog g_ConfigDialog;

/* Special function input mappings. */
static int sdl_loop = 0;

struct input_mapping
{
    char* name;                     /* Human-readable name of emulation special function */
    char* key_mapping;              /* keyboard mapping of function */
    char* joy_config_name;          /* name of joystick mapping in config file */
    GtkWidget* joy_mapping_textbox; /* textbox displaying string representation of joystick mapping */
};

#define mapping_foreach(mapping) for(mapping = mappings; mapping->name; mapping++)

static struct input_mapping mappings[] =
    {
        {
            "Toggle fullscreen",
            "Alt+Enter",
            "Joy Mapping Fullscreen",
            NULL
        },
        {
            "Stop emulation",
            "Esc",
            "Joy Mapping Stop",
            NULL
        },
        {
            "Pause emulation",
            "P",
            "Joy Mapping Pause",
            NULL
        },
        {
            "Save state",
            "F5",
            "Joy Mapping Save State",
            NULL
        },
        {
            "Load state",
            "F7",
            "Joy Mapping Load State",
            NULL
        },
        {
            "Increment save slot",
            "0-9",
            "Joy Mapping Increment Slot",
            NULL
        },
        {
            "Save Screenshot",
            "F12",
            "Joy Mapping Screenshot",
            NULL
        },
        {
            "Mute/ unmute volume",
            "M",
            "Joy Mapping Mute",
            NULL
        },
        {
            "Decrease volume",
            "[",
            "Joy Mapping Decrease Volume",
            NULL
        },
        {
            "Increase volume",
            "]",
            "Joy Mapping Increase Volume",
            NULL
        },
        {
            "Gameshark button",
            "G",
            "Joy Mapping GS Button",
            NULL
        },
        { 0, 0, 0, 0 } /* Last entry. */
    };

static void callback_config_graphics(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo));
    if(name)
        plugin_exec_config(name);
}

static void callback_test_graphics(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo));
    if(name)
        plugin_exec_test(name);
}

static void callback_about_graphics(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo));
    if(name)
        plugin_exec_about(name);
}

static void callback_config_audio(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo));
    if(name)
        plugin_exec_config(name);
}

static void callback_testAudio(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo));
    if(name)
        plugin_exec_test(name);
}

static void callback_aboutAudio(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo));
    if(name)
        plugin_exec_about(name);
}

static void callback_configInput(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo));
    if(name)
        plugin_exec_config(name);
}

static void callback_testInput(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo));
    if(name)
        plugin_exec_test(name);
}

static void callback_aboutInput(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo));
    if(name)
        plugin_exec_about(name);
}

static void callback_configRSP(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo));
    if(name)
        plugin_exec_config(name);
}

static void callback_testRSP(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo));
    if(name)
        plugin_exec_test(name);
}

static void callback_aboutRSP(GtkWidget* widget, gpointer data)
{
    const char* name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo));
    if(name)
        plugin_exec_about(name);
}

/* Add dirname if not in model, otherwise give an error. */
static void addRomDirectory(const gchar* dirname)
{
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    GtkTreeIter iter;
    gchar* text;

    gboolean keepgoing = gtk_tree_model_get_iter_first(model, &iter);

    while(keepgoing)
        {
        gtk_tree_model_get(model, &iter, 0, &text, -1);
        if(!strcmp(dirname, text))
           {
           error_message(tr("The directory you selected is already in the list."));
           return;
           }

        keepgoing = gtk_tree_model_iter_next(model, &iter);
        }

    GtkTreeIter newiter;
    gtk_list_store_append(GTK_LIST_STORE(model), &newiter);
    gtk_list_store_set(GTK_LIST_STORE(model), &newiter, 0, dirname,-1);
}

/* Get a directory name from the user and attempt to add it. */
static void callback_directory_add(GtkWidget* widget, gpointer data)
{
    GtkWidget* file_chooser;

    /* Use standard gtk file chooser to get a directory from the user. */
    file_chooser = gtk_file_chooser_dialog_new(tr("Select Rom Directory"), GTK_WINDOW(g_ConfigDialog.dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

    if(gtk_dialog_run(GTK_DIALOG(file_chooser))==GTK_RESPONSE_ACCEPT)
        {
        char* dirname;
        gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
        dirname = malloc(strlen(path)+2);
        strcpy(dirname, path);

        /* Add trailing '/' */
        if(dirname[strlen(dirname)-1]!='/')
            strcat(dirname, "/");

        addRomDirectory(dirname);

        free(dirname);
        g_free(path);
        }

    gtk_widget_destroy(file_chooser);
}

/* Remove all directory name(s) from model. */
static void callback_directory_remove_all(GtkWidget *widget, gpointer data)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    gtk_tree_selection_select_all(selection);
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

/* Remove selected directory name(s) from model. */
static void callback_directory_remove(GtkWidget *widget, gpointer data)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));

    while(gtk_tree_selection_count_selected_rows(selection) > 0)
        {
        GList *list = NULL, *llist = NULL;
        list = gtk_tree_selection_get_selected_rows(selection, &model);
        llist = g_list_first(list);

        do
            {
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)llist->data))
                gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            }
        while ((llist = g_list_next(llist)));

        g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(list);
        }
}

/* Apply / Ok Button. */
static void callback_apply_changes(GtkWidget* widget, gpointer data)
{
    const char *filename, *name;
    gchar* text = NULL;
    int guivalue, currentvalue;
    gboolean updategui = FALSE, rcsrescan = FALSE;
    int i = 0;

    /* General Tab */ 

    /* CPU Core */
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.coreInterpreterCheckButton)))
        config_put_number("Core", CORE_INTERPRETER); 
#ifndef NO_ASM
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.coreDynaRecCheckButton)))
        config_put_number("Core", CORE_DYNAREC);
#endif
    else
        config_put_number("Core", CORE_PURE_INTERPRETER);

    /* Compatibility */
    guivalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noCompiledJumpCheckButton));
    config_put_bool("NoCompiledJump", guivalue);

    guivalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noMemoryExpansion));
    config_put_bool("NoMemoryExpansion", guivalue);

    /* Miscellaneous */
    guivalue = gtk_combo_box_get_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo));
    currentvalue = config_get_number("ToolbarSize", -1);

    switch(guivalue)
        {
        case 2:
            if(currentvalue!=32)
                {
                config_put_number("ToolbarSize", 32);
                updategui = TRUE;
                }
            break;
        case 0:
            if(currentvalue!=16)
                {
                config_put_number("ToolbarSize", 16);
                updategui = TRUE;
                }
            break;
        default:
            if(currentvalue!=22)
                {
                config_put_number("ToolbarSize", 22);
                updategui = TRUE;
                }
        }

    if(updategui)
        {
        currentvalue = config_get_number("ToolbarSize", 22);
        set_icon(g_MainWindow.openButtonImage, "mupen64cart", currentvalue, TRUE);
        set_icon(g_MainWindow.playButtonImage, "media-playback-start", currentvalue, FALSE);
        set_icon(g_MainWindow.pauseButtonImage, "media-playback-pause", currentvalue, FALSE);
        set_icon(g_MainWindow.stopButtonImage, "media-playback-stop", currentvalue, FALSE);
        set_icon(g_MainWindow.saveStateButtonImage, "document-save", currentvalue, FALSE);
        set_icon(g_MainWindow.loadStateButtonImage, "document-revert", currentvalue, FALSE);
        set_icon(g_MainWindow.fullscreenButtonImage, "view-fullscreen", currentvalue, FALSE);
        set_icon(g_MainWindow.configureButtonImage, "preferences-system", currentvalue, FALSE);
        updategui = FALSE;
        }

    guivalue = gtk_combo_box_get_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo));
    currentvalue = config_get_number("ToolbarStyle", -1);

    if(currentvalue==-1||currentvalue!=guivalue)
        {
        switch(guivalue)
            {
            case 1:
                config_put_number("ToolbarStyle", 1);
                gtk_toolbar_set_style(GTK_TOOLBAR(g_MainWindow.toolBar), GTK_TOOLBAR_TEXT);
                break;
            case 2:
                config_put_number("ToolbarStyle", 2);
                gtk_toolbar_set_style(GTK_TOOLBAR(g_MainWindow.toolBar), GTK_TOOLBAR_BOTH);
                break;
            default:
                config_put_number("ToolbarStyle", 0);
                gtk_toolbar_set_style(GTK_TOOLBAR(g_MainWindow.toolBar), GTK_TOOLBAR_ICONS);
            }
        }

    savestates_set_autoinc_slot(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.autoincSaveSlotCheckButton)));
    config_put_bool("AutoIncSaveSlot", savestates_get_autoinc_slot());

    /* If --noask was specified at the commandline, don't modify config. */
    if(!g_NoaskParam)
        {
        g_Noask = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noaskCheckButton));
        config_put_bool("NoAsk",g_Noask);
        }

    g_OsdEnabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.osdEnabled));
    config_put_bool("OsdEnabled", g_OsdEnabled);

    g_Fullscreen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.alwaysFullscreen));
    config_put_bool("GuiStartFullscreen", g_Fullscreen);

    /* Plugins Tab */

    /* Don't automatically save plugin to config if user specified it at the commandline. */
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo));
    if(!g_GfxPlugin&&name)
        {
        filename = plugin_filename_by_name(name);
        if(filename)
            config_put_string("Gfx Plugin", filename);
        }
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo));
    if(!g_AudioPlugin&&name)
        {
        filename = plugin_filename_by_name(name);
        if(filename)
            config_put_string("Audio Plugin", filename);
        }
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo));
    if(!g_InputPlugin&&name)
        {
        filename = plugin_filename_by_name(name);
        if(filename)
            config_put_string("Input Plugin", filename);
        }
    name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo));
    if(!g_RspPlugin&&name)
        {
        filename = plugin_filename_by_name(name);
        if(filename) config_put_string("RSP Plugin", filename);
        }

    /* Rom Browser Tab */

    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    GtkTreeIter iter;

    /* Get the first item. */
    gboolean keepgoing = gtk_tree_model_get_iter_first(model, &iter);

    /* Iterate through list and add to the configuration if different from current. */
    i = 0;
    while(keepgoing)
        {
        gtk_tree_model_get(model, &iter, 0, &text, -1);

        if(text!=NULL)
            {
            char buffer[32];
            sprintf(buffer, "RomDirectory[%d]", i++);
            if(strncmp(config_get_string(buffer, "/"), text, PATH_MAX)!=0)
                rcsrescan = TRUE;
            config_put_string(buffer, text);
            }

        keepgoing = gtk_tree_model_iter_next(model, &iter);
        }

    currentvalue = config_get_number("NumRomDirs", 0);

    if(currentvalue!=i)
        rcsrescan = TRUE;

    currentvalue = config_get_bool("RomDirsScanRecursive", FALSE);
    guivalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.romDirsScanRecCheckButton));
    config_put_bool("RomDirsScanRecursive", guivalue);

    if(currentvalue!=guivalue||rcsrescan)
        g_romcache.rcstask = RCS_RESCAN;

    config_put_number("NumRomDirs", i);

    currentvalue = config_get_bool("RomBrowserShowFullPaths", FALSE);
    guivalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.romShowFullPathsCheckButton));
    config_put_bool("RomBrowserShowFullPaths", guivalue);

    if(currentvalue!=guivalue)
        rombrowser_refresh(g_romcache.length, TRUE);

    struct input_mapping *mapping;

    /* Update special function input mappings. */
    mapping_foreach(mapping)
        {
        if(mapping->joy_mapping_textbox)
            {
            text = gtk_editable_get_chars(GTK_EDITABLE(mapping->joy_mapping_textbox), 0, -1);
            if(strcmp(text, config_get_string(mapping->joy_config_name, "")) != 0)
                config_put_string(mapping->joy_config_name, text);
            g_free(text);
            }
        }

    /* Hide dialog. */
    if(data)
        gtk_widget_hide(g_ConfigDialog.dialog);
}

/* Initalize configuration dialog. */
void show_configure(void)
{
    int index, i;
    char *name, *combo;

    callback_directory_remove_all(NULL, NULL);

    for(i = 0; i < config_get_number("NumRomDirs", 0); i++)
        {
        char buffer[30];
        sprintf(buffer, "RomDirectory[%d]", i);
        addRomDirectory(config_get_string(buffer, ""));
        }

    if(g_GfxPlugin)
        name = plugin_name_by_filename(g_GfxPlugin);
    else
        name = plugin_name_by_filename(config_get_string("Gfx Plugin", ""));
    if(name)
        {
        index = 0;
        if(g_ConfigDialog.graphicsPluginGList)
            {
            GList* element = g_list_first(g_ConfigDialog.graphicsPluginGList);
            while(element)
                {
                gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo), index);
                combo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo));
                if(strcmp(combo, name)==0)
                    {
                    free(combo);
                    break;
                    }
                free(combo);
                index++;
                element = g_list_next(element);
                }
            }
        }

    if(g_AudioPlugin)
        name = plugin_name_by_filename(g_AudioPlugin);
    else
        name = plugin_name_by_filename(config_get_string("Audio Plugin", ""));
    if(name)
        {
        index = 0;
        if(g_ConfigDialog.audioPluginGList)
            {
            GList *element = g_list_first(g_ConfigDialog.audioPluginGList);
            while(element)
                {
                gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.audioCombo), index);
                combo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo));
                if(strcmp(combo, name)==0)
                    {
                    free(combo);
                    break;
                    }
                free(combo);
                index++;
                element = g_list_next(element);
                }
            }
        }

    if(g_InputPlugin)
        name = plugin_name_by_filename(g_InputPlugin);
    else
        name = plugin_name_by_filename(config_get_string("Input Plugin", ""));
    if(name)
        {
        index = 0;
        if(g_ConfigDialog.inputPluginGList)
            {
            GList *element = g_list_first(g_ConfigDialog.inputPluginGList);
            while(element)
                {
                gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.inputCombo), index);
                combo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo));
                if(strcmp(combo, name)==0)
                    {
                    free(combo);
                    break;
                    }
                free(combo);
                index++;
                element = g_list_next(element);
                }
            }
        }

    if(g_RspPlugin)
        name = plugin_name_by_filename(g_RspPlugin);
    else
        name = plugin_name_by_filename(config_get_string("RSP Plugin", ""));
    if(name)
        {
        index = 0;
        if(g_ConfigDialog.rspPluginGList)
            {
            GList *element = g_list_first(g_ConfigDialog.rspPluginGList);
            while(element)
                {
                gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.rspCombo), index);
                combo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo));
                if(strcmp(combo, name)==0)
                    {
                    free(combo);
                    break;
                    }
                free(combo);
                index++;
                element = g_list_next(element);
                }
            }
        }

#ifdef NO_ASM
    switch(config_get_number("Core", CORE_INTERPRETER))
#else
    switch(config_get_number("Core", CORE_DYNAREC))
#endif
        {
        case CORE_INTERPRETER:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.coreInterpreterCheckButton), 1);
            break;
#ifndef NO_ASM
        case CORE_DYNAREC:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.coreDynaRecCheckButton), 1);
            break;
#endif
        case CORE_PURE_INTERPRETER:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.corePureInterpCheckButton), 1);
            break;
        }

    switch(config_get_number("ToolbarStyle", 0))
        {
        case 0:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), 0);
            break;
        case 1:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), 1);
            break;
        case 2:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), 2);
            break;
        default:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), 0);
        }

    switch(config_get_number("ToolbarSize", 22))
        {
        case 32:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), 2);
            break;
        case 16:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), 0);
            break;
        default:
            gtk_combo_box_set_active(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), 1);
        }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.romDirsScanRecCheckButton), config_get_bool("RomDirsScanRecursive", FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.romShowFullPathsCheckButton), config_get_bool("RomBrowserShowFullPaths", FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noCompiledJumpCheckButton), config_get_bool("NoCompiledJump", FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noMemoryExpansion), config_get_bool("NoMemoryExpansion", FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.autoincSaveSlotCheckButton), config_get_bool("AutoIncSaveSlot", FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.noaskCheckButton), !g_Noask);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.osdEnabled), g_OsdEnabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_ConfigDialog.alwaysFullscreen), config_get_bool("GuiStartFullscreen", FALSE));

    /* If --noask was specified at the commandline, disable checkbox. */
    gtk_widget_set_sensitive(g_ConfigDialog.noaskCheckButton, !g_NoaskParam);

    /* Set special function input mapping textbox values. */
    struct input_mapping *mapping;

    mapping_foreach(mapping)
        {
        if(mapping->joy_mapping_textbox)
            {
            gtk_editable_delete_text(GTK_EDITABLE(mapping->joy_mapping_textbox), 0, -1);
            gtk_editable_insert_text(GTK_EDITABLE(mapping->joy_mapping_textbox), config_get_string(mapping->joy_config_name, ""), strlen(config_get_string(mapping->joy_config_name, "")), &i);
            }
        }

    gtk_window_set_focus(GTK_WINDOW(g_ConfigDialog.dialog), g_ConfigDialog.okButton);
    gtk_widget_show_all(g_ConfigDialog.dialog);
}

static void callback_cancelSetInput(GtkWidget *widget, gint response, GtkWidget *textbox)
{
    // If user clicked the delete button, delete current input mapping
    if(response == GTK_RESPONSE_NO)
        gtk_editable_delete_text(GTK_EDITABLE(textbox), 0, -1);

    sdl_loop = 0;
    gtk_widget_destroy(widget);
}

static void callback_setInput(GtkWidget* widget, GdkEventAny* event, struct input_mapping *mapping)
{
    int i;
    char *mapping_str = NULL;
    char buf[512] = {0};
    SDL_Joystick *joys[10] = {0};
    SDL_Event sdl_event;
    GtkWidget *dialog;
    GtkWidget *label;
    GdkColor color;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        printf("%s: Could not init SDL video or joystick subsystem\n", __FUNCTION__);
        return;
    }

    SDL_JoystickEventState(SDL_ENABLE);

    // open all joystick devices (max of 10)
    for(i = 0; i < SDL_NumJoysticks() && i < 10; i++)
    {
        joys[i] = SDL_JoystickOpen(i);
        if(joys[i] == NULL)
        {
            printf("%s: Could not open joystick %d (%s): %s\n",
                   __FUNCTION__, i, SDL_JoystickName(i), SDL_GetError());
        }
    }

    // highlight mapping to change
    color.red = color.blue = 0;
    color.green = 0xffff;
    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, &color);
    // fix cursor position
    gtk_editable_select_region(GTK_EDITABLE(widget), 0, 0);

    // pop up dialog window telling user what to do
    dialog = gtk_dialog_new_with_buttons(tr("Map Special Function"),
                                         GTK_WINDOW(g_ConfigDialog.dialog),
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
                                         GTK_STOCK_DELETE, GTK_RESPONSE_NO,
                                         NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(callback_cancelSetInput),
                     widget);

    snprintf(buf, 512, tr("Press a controller button for:\n\"%s\""), mapping->name);
    label = gtk_label_new(buf);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_size_request(label, 165, -1);

    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all(dialog);

    // capture joystick input from user
    sdl_loop = 1;
    while(sdl_loop)
    {
        // let gtk work if it needs to. Need this so user can click buttons on the dialog.
        gdk_threads_leave();
        while(g_main_context_iteration(NULL, FALSE));
        gdk_threads_enter();

        if(SDL_PollEvent(&sdl_event))
        {
            // only accept joystick events
            if(sdl_event.type == SDL_JOYAXISMOTION ||
               sdl_event.type == SDL_JOYBUTTONDOWN ||
               sdl_event.type == SDL_JOYHATMOTION)
            {
                if((mapping_str = event_to_str(&sdl_event)) != NULL)
                {
                    // change textbox to reflect new joystick input mapping
                    gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
                    gtk_editable_insert_text(GTK_EDITABLE(widget), mapping_str, strlen(mapping_str), &i);

                    struct input_mapping *mapping;

                    // if another function has the same input mapped to it, remove it
                    mapping_foreach(mapping)
                    {
                        gchar *text;

                        if(mapping->joy_mapping_textbox &&
                           mapping->joy_mapping_textbox != widget)
                        {
                            text = gtk_editable_get_chars(GTK_EDITABLE(mapping->joy_mapping_textbox), 0, -1);
                            if(strcmp(text, mapping_str) == 0)
                                gtk_editable_delete_text(GTK_EDITABLE(mapping->joy_mapping_textbox), 0, -1);

                            g_free(text);
                        }
                    }

                    free(mapping_str);

                    sdl_loop = 0;
                    gtk_widget_destroy(dialog);
                }
            }
        }
        usleep(50000); // don't melt the cpu
    }

    // remove highlight on mapping to change
    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, NULL);

    SDL_Quit();
}

/* Create main configure dialog. */
void create_configDialog(void)
{
    GtkWidget *label;
    GtkWidget *frame;
    GtkWidget *button, *button_config, *button_test, *button_about;
    GtkWidget *vbox;
    GtkWidget *hbox1, *hbox2;

    list_node_t *node;
    plugin *p;

    /* Since we rebuild the plugin lists, clear them if present. */
    if(g_ConfigDialog.graphicsPluginGList!=NULL)
        {
        g_list_free(g_ConfigDialog.graphicsPluginGList);
        g_ConfigDialog.graphicsPluginGList = NULL;
        }
    if(g_ConfigDialog.audioPluginGList!=NULL)
        {
        g_list_free(g_ConfigDialog.audioPluginGList);
        g_ConfigDialog.audioPluginGList = NULL;
        }
    if(g_ConfigDialog.inputPluginGList==NULL)
        {
        g_list_free(g_ConfigDialog.inputPluginGList);
        g_ConfigDialog.inputPluginGList = NULL;
        }
    if(g_ConfigDialog.rspPluginGList==NULL)
        {
        g_list_free(g_ConfigDialog.rspPluginGList);
        g_ConfigDialog.rspPluginGList = NULL;
        }

    /* Iterate through plugins, add only plugin if specified, otherwise add all plugins. */
    list_foreach(g_PluginList, node)
        {
        p = (plugin *)node->data;

        switch(p->type)
            {
            case PLUGIN_TYPE_GFX:
                if(!g_GfxPlugin||(g_GfxPlugin&&(strcmp(g_GfxPlugin, p->file_name)==0)))
                    g_ConfigDialog.graphicsPluginGList = g_list_append(g_ConfigDialog.graphicsPluginGList, p->plugin_name);
                break;
            case PLUGIN_TYPE_AUDIO:
                if(!g_AudioPlugin||(g_AudioPlugin &&(strcmp(g_AudioPlugin, p->file_name)==0)))
                    g_ConfigDialog.audioPluginGList = g_list_append(g_ConfigDialog.audioPluginGList, p->plugin_name);
                break;
            case PLUGIN_TYPE_CONTROLLER:
                if(!g_InputPlugin||(g_InputPlugin&&(strcmp(g_InputPlugin, p->file_name)==0)))
                    g_ConfigDialog.inputPluginGList = g_list_append(g_ConfigDialog.inputPluginGList, p->plugin_name);
                break;
            case PLUGIN_TYPE_RSP:
                if(!g_RspPlugin||(g_RspPlugin&&(strcmp(g_RspPlugin, p->file_name)==0)))
                    g_ConfigDialog.rspPluginGList = g_list_append(g_ConfigDialog.rspPluginGList, p->plugin_name);
                break;
            }
        }

    /* Create dialog window. */
    g_ConfigDialog.dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(g_ConfigDialog.dialog), tr("Configure"));
    gtk_window_set_transient_for(GTK_WINDOW(g_ConfigDialog.dialog), GTK_WINDOW(g_MainWindow.window));
    g_signal_connect(g_ConfigDialog.dialog, "delete_event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    /* Create notebook, i.e. tabs. */
    g_ConfigDialog.notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(g_ConfigDialog.notebook), 10);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(g_ConfigDialog.notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(g_ConfigDialog.dialog)->vbox), g_ConfigDialog.notebook, TRUE, TRUE, 0);

    /* Apply / Ok / Cancel buttons. */
    button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(g_ConfigDialog.dialog)->action_area), button, TRUE, TRUE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(callback_apply_changes), (gpointer)FALSE);

    button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(g_ConfigDialog.dialog)->action_area), button, TRUE, TRUE, 0);
     g_signal_connect_object(button, "clicked", G_CALLBACK(gtk_widget_hide_on_delete), g_ConfigDialog.dialog, G_CONNECT_SWAPPED);

    g_ConfigDialog.okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(g_ConfigDialog.dialog)->action_area), g_ConfigDialog.okButton, TRUE, TRUE, 0);
    g_signal_connect(g_ConfigDialog.okButton, "clicked", G_CALLBACK(callback_apply_changes), (gpointer)TRUE);

    /* Create main settings configuration page. */

    label = gtk_label_new(tr("Main Settings"));
    g_ConfigDialog.configMupen = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_ConfigDialog.configMupen), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ConfigDialog.notebook), g_ConfigDialog.configMupen, label);

    /* Create frame and CPU core options. */
    frame = gtk_frame_new(tr("CPU Core"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configMupen), frame, TRUE, TRUE, 0);
    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    g_ConfigDialog.coreInterpreterCheckButton = gtk_radio_button_new_with_mnemonic(NULL, tr("_Interpreter"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.coreInterpreterCheckButton, FALSE, FALSE, 0);
#ifndef NO_ASM
    g_ConfigDialog.coreDynaRecCheckButton = gtk_radio_button_new_with_mnemonic(gtk_radio_button_group(GTK_RADIO_BUTTON(g_ConfigDialog.coreInterpreterCheckButton)), tr("_Dynamic recompiler"));
     gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.coreDynaRecCheckButton, FALSE, FALSE, 0);
#endif
    g_ConfigDialog.corePureInterpCheckButton = gtk_radio_button_new_with_mnemonic(gtk_radio_button_group(GTK_RADIO_BUTTON(g_ConfigDialog.coreInterpreterCheckButton)), tr("_Pure interpreter"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.corePureInterpCheckButton, FALSE, FALSE, 0);

    /* Create frame and compatibility options. */
    frame = gtk_frame_new(tr("Compatibility"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configMupen), frame, TRUE, TRUE, 0);
    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    g_ConfigDialog.noCompiledJumpCheckButton = gtk_check_button_new_with_mnemonic("Disable compiled _jump");
    g_ConfigDialog.noMemoryExpansion = gtk_check_button_new_with_mnemonic("Disable memory _expansion");
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.noCompiledJumpCheckButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.noMemoryExpansion, FALSE, FALSE, 0);

    /* Create frame and miscellaneous options. */
    frame = gtk_frame_new(tr("Miscellaneous"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configMupen), frame, TRUE, TRUE, 0);
    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    hbox1 = gtk_hbox_new(TRUE, 0);

    g_ConfigDialog.toolbarStyleCombo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), (char*)tr("Icons"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), (char*)tr("Text"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarStyleCombo), (char*)tr("Icons & Text"));

    label = gtk_label_new_with_mnemonic(tr("Toolbar st_yle:"));
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.toolbarStyleCombo);
    gtk_box_pack_start(GTK_BOX(hbox1), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.toolbarStyleCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

    hbox1 = gtk_hbox_new(TRUE, 0);

    g_ConfigDialog.toolbarSizeCombo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), (char *)tr("Small"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), (char *)tr("Medium"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.toolbarSizeCombo), (char *)tr("Large"));

    label = gtk_label_new_with_mnemonic(tr("Toolbar si_ze:"));
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.toolbarSizeCombo);
    gtk_box_pack_start(GTK_BOX(hbox1), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.toolbarSizeCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

    g_ConfigDialog.autoincSaveSlotCheckButton = gtk_check_button_new_with_mnemonic(tr("A_uto increment save slot"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.autoincSaveSlotCheckButton, FALSE, FALSE, 0);

    g_ConfigDialog.noaskCheckButton = gtk_check_button_new_with_mnemonic(tr("Ask _before loading bad dump/ hacked rom"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.noaskCheckButton, FALSE, FALSE, 0);

    g_ConfigDialog.osdEnabled = gtk_check_button_new_with_mnemonic(tr("On _screen display enabled"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.osdEnabled, FALSE, FALSE, 0);

    g_ConfigDialog.alwaysFullscreen = gtk_check_button_new_with_mnemonic(tr("Always start in _full screen mode"));
    gtk_box_pack_start(GTK_BOX(vbox), g_ConfigDialog.alwaysFullscreen, FALSE, FALSE, 0);

    /* Create plugin configuration page. */

    label = gtk_label_new(tr("Plugins"));
    g_ConfigDialog.configPlugins = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_ConfigDialog.configPlugins), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ConfigDialog.notebook), g_ConfigDialog.configPlugins, label);

    /* Graphics plugin area. */
    label = gtk_label_new_with_mnemonic(tr("_Graphics Plugin"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configPlugins), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    g_ConfigDialog.graphicsCombo = gtk_combo_box_new_text();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.graphicsCombo);
    if(g_ConfigDialog.graphicsPluginGList)
        {
        GList *element = g_list_first(g_ConfigDialog.graphicsPluginGList);
        while(element)
            {
            gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.graphicsCombo), (gchar*)g_list_nth_data(element, 0));
            element = g_list_next(element);
            }
        }
    else
        gtk_widget_set_sensitive(GTK_WIDGET(g_ConfigDialog.graphicsCombo), FALSE);

    button_config = gtk_button_new_with_label(tr("Config"));
    button_test = gtk_button_new_with_label(tr("Test"));
    button_about = gtk_button_new_with_label(tr("About"));
    g_signal_connect(button_config, "clicked", G_CALLBACK(callback_config_graphics), NULL);
    g_signal_connect(button_test, "clicked", G_CALLBACK(callback_test_graphics), NULL);
    g_signal_connect(button_about, "clicked", GTK_SIGNAL_FUNC(callback_about_graphics), NULL);

    g_ConfigDialog.graphicsImage = gtk_image_new();
    set_icon(g_ConfigDialog.graphicsImage, "video-display", 32, FALSE);

    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.graphicsImage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.graphicsCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_config, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_test, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_about, TRUE, TRUE, 0);

    /* Audio plugin area */
    label = gtk_label_new_with_mnemonic(tr("A_udio Plugin"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configPlugins), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    g_ConfigDialog.audioCombo = gtk_combo_box_new_text();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.audioCombo);
    if(g_ConfigDialog.audioPluginGList)
        {
        GList* element = g_list_first(g_ConfigDialog.audioPluginGList);
        while(element)
            {
            gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.audioCombo), (gchar*)g_list_nth_data(element, 0));
            element = g_list_next(element);
            }
        }
    else
        gtk_widget_set_sensitive(GTK_WIDGET(g_ConfigDialog.audioCombo), FALSE);

    button_config = gtk_button_new_with_label(tr("Config"));
    button_test = gtk_button_new_with_label(tr("Test"));
    button_about = gtk_button_new_with_label(tr("About"));
    g_signal_connect(button_config, "clicked", G_CALLBACK(callback_config_audio), NULL);
    g_signal_connect(button_test, "clicked", G_CALLBACK(callback_testAudio), NULL);
    g_signal_connect(button_about, "clicked", G_CALLBACK(callback_aboutAudio), NULL);

    g_ConfigDialog.audioImage = gtk_image_new();
    set_icon(g_ConfigDialog.audioImage, "audio-card", 32, FALSE);

    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.audioImage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.audioCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_config, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_test, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_about, TRUE, TRUE, 0);

    /* Input plugin area. */
    label = gtk_label_new_with_mnemonic(tr("_Input Plugin"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configPlugins), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    g_ConfigDialog.inputCombo = gtk_combo_box_new_text();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.inputCombo);
    if(g_ConfigDialog.inputPluginGList)
        {
        GList* element = g_list_first(g_ConfigDialog.inputPluginGList);
        while(element)
        {
            gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.inputCombo), (gchar*)g_list_nth_data(element, 0));
            element = g_list_next(element);
            }
        }
    else
        gtk_widget_set_sensitive(GTK_WIDGET(g_ConfigDialog.inputCombo), FALSE);

    button_config = gtk_button_new_with_label(tr("Config"));
    button_test = gtk_button_new_with_label(tr("Test"));
    button_about = gtk_button_new_with_label(tr("About"));
    g_signal_connect(button_config, "clicked", G_CALLBACK(callback_configInput), NULL);
    g_signal_connect(button_test, "clicked", G_CALLBACK(callback_testInput), NULL);
    g_signal_connect(button_about, "clicked", G_CALLBACK(callback_aboutInput), NULL);

    g_ConfigDialog.inputImage = gtk_image_new();
    set_icon(g_ConfigDialog.inputImage, "input-gaming", 32, FALSE);

    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.inputImage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.inputCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_config, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_test, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_about, TRUE, TRUE, 0);

    /* RSP plugin area. */
     label = gtk_label_new_with_mnemonic(tr("_RSP Plugin"));
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configPlugins), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    hbox1 = gtk_hbox_new(FALSE, 5);
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    g_ConfigDialog.rspCombo = gtk_combo_box_new_text();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), g_ConfigDialog.rspCombo);
    if(g_ConfigDialog.rspPluginGList)
        {
        GList* element = g_list_first(g_ConfigDialog.rspPluginGList);
        while(element)
            {
            gtk_combo_box_append_text(GTK_COMBO_BOX(g_ConfigDialog.rspCombo), (gchar*)g_list_nth_data(element, 0));
            element = g_list_next(element);
            }
        }
    else
        gtk_widget_set_sensitive(GTK_WIDGET(g_ConfigDialog.rspCombo), FALSE);

    button_config = gtk_button_new_with_label(tr("Config"));
    button_test = gtk_button_new_with_label(tr("Test"));
    button_about = gtk_button_new_with_label(tr("About"));
    g_signal_connect(button_config, "clicked", G_CALLBACK(callback_configRSP), NULL);
    g_signal_connect(button_test, "clicked", G_CALLBACK(callback_testRSP), NULL);
    g_signal_connect(button_about, "clicked", G_CALLBACK(callback_aboutRSP), NULL);

    g_ConfigDialog.rspImage = gtk_image_new();
    set_icon(g_ConfigDialog.rspImage, "cpu", 32, TRUE);

    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.rspImage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), g_ConfigDialog.rspCombo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_config, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_test, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), button_about, TRUE, TRUE, 0);

    /* Create the rom settings page. */

    label = gtk_label_new(tr("Rom Browser"));
    g_ConfigDialog.configRomBrowser = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(g_ConfigDialog.configRomBrowser), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ConfigDialog.notebook), g_ConfigDialog.configRomBrowser, label);

    frame = gtk_frame_new(tr("Rom Directories"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configRomBrowser), frame, TRUE, TRUE, 0);

    hbox1 = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox1), 10);
    gtk_container_add(GTK_CONTAINER(frame), hbox1);

    /* Create a new rom list. */
    g_ConfigDialog.romDirectoryList = gtk_tree_view_new();

    /* Get the GtkTreeSelection and flip the mode to GTK_SELECTION_MULTIPLE. */
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    /* Create our model and set it. */
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList), -1, "Directory", renderer, "text", 0, NULL);
    GtkTreeModel* model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));

    gtk_tree_view_set_model(GTK_TREE_VIEW(g_ConfigDialog.romDirectoryList), model);
    g_object_unref(model);

    /* Create a scrolled window to contain the rom list, make scrollbar visibility automatic. */
    GtkWidget* scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* Add the romDirectoryList tree view into our scrolled window. */
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), g_ConfigDialog.romDirectoryList);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_ETCHED_IN);

    /* Create a new vertical button box with top alignment. */
    vbox = gtk_vbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbox), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(vbox), 5);

    button = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(callback_directory_add), NULL);

    button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(callback_directory_remove), NULL);

    button = gtk_button_new_with_mnemonic(tr("Remo_val All"));
    gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_REMOVE,  GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(callback_directory_remove_all), NULL);

    gtk_box_pack_start(GTK_BOX(hbox1), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), vbox, FALSE, FALSE, 0);

    g_ConfigDialog.romDirsScanRecCheckButton = gtk_check_button_new_with_mnemonic(tr("Recursively _scan rom directories"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configRomBrowser), g_ConfigDialog.romDirsScanRecCheckButton, FALSE, FALSE, 0);

    g_ConfigDialog.romShowFullPathsCheckButton = gtk_check_button_new_with_mnemonic(tr("Show _full paths in filenames"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configRomBrowser), g_ConfigDialog.romShowFullPathsCheckButton, FALSE, FALSE, 0);

    /* Create hotkey configuration page. */

    label = gtk_label_new("Hotkeys");
    g_ConfigDialog.configInputMappings = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_ConfigDialog.configInputMappings), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_ConfigDialog.notebook), g_ConfigDialog.configInputMappings, label);

    struct input_mapping* mapping;
    GtkTooltips* tooltips;

    tooltips = gtk_tooltips_new();

    frame = gtk_frame_new(tr("Input Mappings"));
    gtk_box_pack_start(GTK_BOX(g_ConfigDialog.configInputMappings), frame, TRUE, TRUE, 0);

    hbox1 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), hbox1);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Create column of all mapping names. */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), FALSE, FALSE, 0);
    mapping_foreach(mapping)
        gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(tr(mapping->name)), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox1), vbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Create column of all keyboard shortcuts. */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(tr("Keyboard")), FALSE, FALSE, 0);
    mapping_foreach(mapping)
        gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(mapping->key_mapping), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox1), vbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Create column of joystick mappings. */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(tr("Controller")), FALSE, FALSE, 0);
    mapping_foreach(mapping)
        {
        mapping->joy_mapping_textbox = gtk_entry_new();
        gtk_widget_set_size_request(mapping->joy_mapping_textbox, 5, -1);
        gtk_editable_set_editable(GTK_EDITABLE(mapping->joy_mapping_textbox), FALSE);
        g_signal_connect(GTK_OBJECT(mapping->joy_mapping_textbox), "button-release-event", GTK_SIGNAL_FUNC(callback_setInput), mapping);

        gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), mapping->joy_mapping_textbox, tr("Click to change"), "");

        gtk_box_pack_start(GTK_BOX(vbox), mapping->joy_mapping_textbox, FALSE, FALSE, 0);
        }

    gtk_box_pack_start(GTK_BOX(hbox1), vbox, TRUE, TRUE, 0);
}

