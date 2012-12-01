/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - ui_disasm_list.c                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 DarkJezter                                         *
 *   Copyright (C) 2004 Tim-Philipp Muller                                 *
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

#ifndef __UI_DISASM_LIST_H__
#define __UI_DISASM_LIST_H__

#include <gtk/gtk.h>

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define TYPE_DISASM_LIST            (disasm_list_get_type ())
#define DISASM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DISASM_LIST, DisasmList))
#define DISASM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_DISASM_LIST, DisasmListClass))
#define DISASM_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DISASM_LIST))
#define DISASM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_DISASM_LIST))
#define DISASM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_DISASM_LIST, DisasmListClass))

/* The data columns that we export via the tree model interface */

enum
{
  DISASM_LIST_COL_RECORD = 0,
  DISASM_LIST_COL_NAME,
  DISASM_LIST_COL_YEAR_BORN,
  DISASM_LIST_N_COLUMNS,
} ;


typedef struct _DisasmList       DisasmList;
typedef struct _DisasmListClass  DisasmListClass;



/* DisasmList: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct _DisasmList
{
  GObject         parent;      /* this MUST be the first member */

  guint           startAddr;    /* number of rows that we have   */

  gint            stamp;       /* Random integer to check whether an iter belongs to our model */
};



/* DisasmListClass: more boilerplate GObject stuff */

struct _DisasmListClass
{
  GObjectClass parent_class;
};


GType             disasm_list_get_type (void);

DisasmList       *disasm_list_new (void);

void disasm_list_update (GtkTreeModel *tree_model, guint address);

gboolean disasm_list_get_iter (GtkTreeModel *tree_model,
              GtkTreeIter  *iter, GtkTreePath  *path);

#endif

