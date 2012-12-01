/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - debugger.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 HyperHacker                                        *
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

#include <glib.h>

#include "debugger.h"

#include "../../main.h"
#include "../../config.h"

GdkColor    color_modif,    // Color of modified register.
                   color_ident;    // Unchanged register.

GtkWidget   *winRegisters;

PangoFontDescription *debugger_font_desc;

void init_debugger_frontend()
{
    color_modif.red = 0x8000;
    color_modif.green = 0xD000;
    color_modif.blue = 0xFFFF;

    color_ident.red = 0xFFFF;
    color_ident.green = 0xFFFF;
    color_ident.blue = 0xFFFF;

    debugger_font_desc = pango_font_description_from_string("Monospace 9");

    gdk_threads_enter();
    init_registers();
    gdk_threads_leave();

    gdk_threads_enter();
    init_desasm();
    gdk_threads_leave();
    /*
    gdk_threads_enter();
    init_breakpoints();
    gdk_threads_leave();
    
    gdk_threads_enter();
    init_memedit();
    gdk_threads_leave();
    
    gdk_threads_enter();
    init_varlist();
    gdk_threads_leave();

    gdk_threads_enter();
    init_TLBwindow();
    gdk_threads_leave();
    */

    if(dynacore==CORE_DYNAREC)
      error_message("You are trying to use the debugger with the dynamic-recompiler.  This is unfinished, and many features of the debugger WILL NOT WORK PROPERLY.  If you have a bug you'd like to report, try it first using the debugger with either of the interpreted cores.");

}


void update_debugger_frontend( uint32 pc )
// Update debugger state and display.
{
    //printf("Update_debugger_frontend( %08x );\n",pc);

    if(registers_opened) {
        gdk_threads_enter();
        update_registers();
        gdk_threads_leave();
    }
    if(desasm_opened) {
        gdk_threads_enter();
        update_disassembler( pc );
        gdk_threads_leave();
    }
    if(regTLB_opened) {
        gdk_threads_enter();
        update_TLBwindow();
        gdk_threads_leave();
    }
    if(memedit_opened) {
        gdk_threads_enter();
        update_memory_editor();
        gdk_threads_leave();
    }
    if(varlist_opened) {
        gdk_threads_enter();
        update_varlist();
        gdk_threads_leave();
    }
}


//Runs each VI for auto-updating views
void debugger_frontend_vi()
{
    if(memedit_auto_update && memedit_opened) {
        gdk_threads_enter();
        update_memory_editor();
        gdk_threads_leave();
    }
    
    if(varlist_auto_update && varlist_opened) {
        gdk_threads_enter();
        update_varlist();
        gdk_threads_leave();
    }
}

