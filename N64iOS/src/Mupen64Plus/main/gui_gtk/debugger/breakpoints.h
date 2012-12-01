/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - breakpoints.h                                           *
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

#ifndef __GTK_BREAKPOINTS_H__
#define __GTK_BREAKPOINTS_H__

#include <gtk/gtk.h>

#include "debugger.h"
#include "ui_clist_edit.h"

int breakpoints_opened;
int breakpoints_editing; /* 1 when editing a breakpoint, 0 when adding. */

GtkWidget* winBreakpoints;

void init_breakpoints();
void get_breakpoint_display_string(char* buf, breakpoint* bpt);
int add_breakpoint(uint32 address);
void update_breakpoints();

#endif  /* __GTK_BREAKPOINTS_H__ */

