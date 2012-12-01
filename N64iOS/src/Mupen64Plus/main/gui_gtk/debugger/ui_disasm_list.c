
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

#include "ui_disasm_list.h"
#include "debugger.h"

#include "../../../debugger/memory.h"
#include "../../../debugger/decoder.h"

static void         disasm_list_init            (DisasmList      *pkg_tree);
static void         disasm_list_class_init      (DisasmListClass *klass);
static void         disasm_list_tree_model_init (GtkTreeModelIface *iface);
static void         disasm_list_finalize        (GObject           *object);
static GtkTreeModelFlags disasm_list_get_flags  (GtkTreeModel      *tree_model);
static gint         disasm_list_get_n_columns   (GtkTreeModel      *tree_model);
static GType        disasm_list_get_column_type (GtkTreeModel      *tree_model,
                                                 gint               index);

/*static gboolean     disasm_list_get_iter        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreePath       *path);
*///moved to ui_disasm_list.h
static GtkTreePath *disasm_list_get_path        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static void         disasm_list_get_value       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 gint               column,
                                                 GValue            *value);

static gboolean     disasm_list_iter_next       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gboolean     disasm_list_iter_children   (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent);

static gboolean     disasm_list_iter_has_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gint         disasm_list_iter_n_children (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gboolean     disasm_list_iter_nth_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent,
                                                 gint               n);

static gboolean     disasm_list_iter_parent     (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *child);



static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */


#define PRELINES    5
#define POSTLINES   20
/*****************************************************************************
 *
 *  disasm_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

GType
disasm_list_get_type (void)
{
  static GType disasm_list_type = 0;

  /* Some boilerplate type registration stuff */
  if (disasm_list_type == 0)
  {
    static const GTypeInfo disasm_list_info =
    {
      sizeof (DisasmListClass),
      NULL,                                         /* base_init */
      NULL,                                         /* base_finalize */
      (GClassInitFunc) disasm_list_class_init,
      NULL,                                         /* class finalize */
      NULL,                                         /* class_data */
      sizeof (DisasmList),
      0,                                           /* n_preallocs */
      (GInstanceInitFunc) disasm_list_init
    };
    static const GInterfaceInfo tree_model_info =
    {
      (GInterfaceInitFunc) disasm_list_tree_model_init,
      NULL,
      NULL
    };

    /* First register the new derived type with the GObject type system */
    disasm_list_type = g_type_register_static (G_TYPE_OBJECT, "DisasmList",
                                               &disasm_list_info, (GTypeFlags)0);

    /* Now register our GtkTreeModel interface with the type system */
    g_type_add_interface_static (disasm_list_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
  }

  return disasm_list_type;
}


/*****************************************************************************
 *
 *  disasm_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void
disasm_list_class_init (DisasmListClass *klass)
{
  GObjectClass *object_class;

  parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = disasm_list_finalize;
}

/*****************************************************************************
 *
 *  disasm_list_tree_model_init: init callback for the interface registration
 *                               in disasm_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void
disasm_list_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = disasm_list_get_flags;
  iface->get_n_columns   = disasm_list_get_n_columns;
  iface->get_column_type = disasm_list_get_column_type;
  iface->get_iter        = disasm_list_get_iter;
  iface->get_path        = disasm_list_get_path;
  iface->get_value       = disasm_list_get_value;
  iface->iter_next       = disasm_list_iter_next;
  iface->iter_children   = disasm_list_iter_children;
  iface->iter_has_child  = disasm_list_iter_has_child;
  iface->iter_n_children = disasm_list_iter_n_children;
  iface->iter_nth_child  = disasm_list_iter_nth_child;
  iface->iter_parent     = disasm_list_iter_parent;
}


/*****************************************************************************
 *
 *  disasm_list_init: this is called everytime a new Disasm list object
 *                    instance is created (we do that in disasm_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void
disasm_list_init (DisasmList *disasm_list)
{
  g_assert (DISASM_LIST_N_COLUMNS == 3);
  disasm_list->startAddr = 0xA4000040;
  disasm_list->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

}


/*****************************************************************************
 *
 *  disasm_list_finalize: this is called just before a Disasm list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void
disasm_list_finalize (GObject *object)
{
/*  DisasmList *disasm_list = DISASM_LIST(object); */

  /* must chain up - finalize parent */
  (* parent_class->finalize) (object);
}


/*****************************************************************************
 *
 *  disasm_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags
disasm_list_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (DISASM_IS_LIST(tree_model), (GtkTreeModelFlags)0);

  return (0);//GTK_TREE_MODEL_ITERS_PERSIST);
}


/*****************************************************************************
 *
 *  disasm_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint
disasm_list_get_n_columns (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (DISASM_IS_LIST(tree_model), 0);

  return 3;
}


/*****************************************************************************
 *
 *  disasm_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType
disasm_list_get_column_type (GtkTreeModel *tree_model,
                             gint          index)
{
  g_return_val_if_fail (DISASM_IS_LIST(tree_model), 0);

  return G_TYPE_STRING;
}


/*****************************************************************************
 *
 *  disasm_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our DisasmRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

gboolean
disasm_list_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter,
                      GtkTreePath  *path)
{
  DisasmList    *disasm_list;
  gint          *indices, depth;

  g_assert(DISASM_IS_LIST(tree_model));
  g_assert(path!=NULL);

  disasm_list = DISASM_LIST(tree_model);

  indices = gtk_tree_path_get_indices(path);
  depth   = gtk_tree_path_get_depth(path);

  if( (dynacore != 1 && depth > 1) || depth > 2 )
      return FALSE;

  //g_assert(record->pos == n);

  // We store a pointer to our Disasm record in the iter
  iter->stamp      = disasm_list->stamp;
  iter->user_data  = (gpointer) (long) ((indices[0]-PRELINES)*4 + (DISASM_LIST(tree_model)->startAddr));

  if(depth==2)
      iter->user_data2 = (gpointer) (long) indices[1];
  else
      iter->user_data2 = (gpointer) -1;

  return TRUE;
}


/*****************************************************************************
 *
 *  disasm_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *
disasm_list_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter)
{
  GtkTreePath  *path;
  DisasmList   *disasm_list;

  g_return_val_if_fail (DISASM_IS_LIST(tree_model), NULL);
  g_return_val_if_fail (iter != NULL,               NULL);
  g_return_val_if_fail (iter->user_data2 != NULL,    NULL);

  disasm_list = DISASM_LIST(tree_model);

  path = gtk_tree_path_new();

  gtk_tree_path_append_index(path, ((unsigned int)(long)(iter->user_data - disasm_list->startAddr)) / 4 + PRELINES);

  if(((long)iter->user_data2)!=-1)
      gtk_tree_path_append_index(path, ((uint32)(long)iter->user_data2));

  return path;
}


/*****************************************************************************
 *
 *  disasm_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
disasm_list_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter  *iter,
                       gint          column,
                       GValue       *value)
{
  char opcode[64];
  char args[128];
  uint32 instr;
  long laddr;

  g_return_if_fail (DISASM_IS_LIST (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (column < 3);

  g_value_init (value, disasm_list_get_column_type(tree_model,column));

  if(((long)iter->user_data2)==-1)
    switch(column)
    {
    case 0:
      sprintf(opcode, "%08lX", (long)iter->user_data);
      g_value_set_string(value, opcode);
      break;
    case 1:
    case 2:
      if((get_memory_flags((uint32)(long)iter->user_data) & MEM_FLAG_READABLE) != 0)
    {
      instr = read_memory_32((uint32)(long)iter->user_data);
      r4300_decode_op( instr, opcode, args, (long)iter->user_data );
    }
      else
    {
      strcpy( opcode, "X+X+X+X");
      sprintf( args, "UNREADABLE (%d)",get_memory_type((uint32)(long)iter->user_data));
    }
      if(column==1)
    g_value_set_string(value, opcode);
      else
    g_value_set_string(value, args);

      break;
    default:
      g_value_set_string(value, "xxx");
    }
  else
    switch(column)
    {
    case 0:
      laddr = (long) get_recompiled_addr((uint32)(long) iter->user_data, (int)(long) iter->user_data2);
      if (sizeof(void *) == 4)
        sprintf(opcode, "[%08lX]", laddr);
      else
        sprintf(opcode, "[%016lX]", laddr);
      g_value_set_string(value, opcode);
      break;
    case 1:
      g_value_set_string(value, (char *)get_recompiled_opcode((uint32)(long) iter->user_data, (uint32)(long) iter->user_data2));
      break;
    case 2:
      g_value_set_string(value, (char *)get_recompiled_args((uint32)(long) iter->user_data, (uint32)(long) iter->user_data2));
      break;
    }
}


/*****************************************************************************
 *
 *  disasm_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean
disasm_list_iter_next (GtkTreeModel  *tree_model,
                       GtkTreeIter   *iter)
{
  DisasmList    *disasm_list;

  g_return_val_if_fail (DISASM_IS_LIST (tree_model), FALSE);

  if (iter == NULL)
    return FALSE;

  disasm_list = DISASM_LIST(tree_model);

  // Is this a disassembly line, or recompiler disassemby line?
  if ((long) iter->user_data2 == -1)
  {
    if ((long) iter->user_data >= disasm_list->startAddr+(POSTLINES*4) || (long) iter->user_data >= 0xFFFFFFFC)
      {
    //printf("Disasm end addr: %016x\n", iter->user_data);
        return FALSE;
      }
    iter->stamp    = disasm_list->stamp;
    iter->user_data+= 4;
  }
  else // recompiler line
  {
    if ((long) iter->user_data2 + 1 >= get_num_recompiled((uint32)(long) iter->user_data))
        return FALSE;
    iter->stamp    = disasm_list->stamp;
    iter->user_data2++;
  }
  return TRUE;
}


/*****************************************************************************
 *
 *  disasm_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
disasm_list_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreeIter  *parent)
{
  DisasmList  *disasm_list;

  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  g_return_val_if_fail (DISASM_IS_LIST (tree_model), FALSE);
  disasm_list = DISASM_LIST(tree_model);

  if (parent==NULL)
  {
    /* parent == NULL is a special case; we need to return the first top-level row */

    /* Set iter to first item in list */
    iter->stamp     = disasm_list->stamp;
    iter->user_data = (gpointer)(long) disasm_list->startAddr-PRELINES;
    iter->user_data2= (gpointer) -1;
  }
  else if ((long) parent->user_data2 == -1)
  {
    iter->stamp     = disasm_list->stamp;
    iter->user_data = parent->user_data;
    iter->user_data2= (uint32*)0;
  }
  else
      return FALSE;

  return TRUE;
}


/*****************************************************************************
 *
 *  disasm_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean
disasm_list_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
  if ((long) iter->user_data2 == -1)
    return get_has_recompiled((uint32)(long) iter->user_data);
  return FALSE;
}


/*****************************************************************************
 *
 *  disasm_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint
disasm_list_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter)
{
  DisasmList  *disasm_list;
  printf("DEBUG: list_iter_n_children called\n");
  g_return_val_if_fail (DISASM_IS_LIST (tree_model), -1);
  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);

  disasm_list = DISASM_LIST(tree_model);

  /* special case: if iter == NULL, return number of top-level rows */
  if (!iter)
      return 0x10;//disasm_list->num_rows;
  else if ((long) iter->user_data2 == -1)
      return get_num_recompiled((uint32)(long) iter->user_data);

  return 0;
}


/*****************************************************************************
 *
 *  disasm_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
disasm_list_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            GtkTreeIter  *parent,
                            gint          n)
{
  DisasmList    *disasm_list;

  g_return_val_if_fail (DISASM_IS_LIST (tree_model), FALSE);

  disasm_list = DISASM_LIST(tree_model);

  if(parent==NULL)
  {
      iter->stamp = disasm_list->stamp;
      iter->user_data = (gpointer)(long) (disasm_list->startAddr - PRELINES + n*4);
      iter->user_data2= (gpointer) -1;
  }
  else if((long) parent->user_data2 == -1)
  {
      iter->stamp = disasm_list->stamp;
      iter->user_data = parent->user_data;
      iter->user_data2= (gpointer)(long) n;
  }
  else
      return FALSE;
  return TRUE;
}


/*****************************************************************************
 *
 *  disasm_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean
disasm_list_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         GtkTreeIter  *child)
{
  if (child == NULL || (long) child->user_data2 == -1)
      return FALSE;

  DisasmList    *disasm_list;

  g_return_val_if_fail (DISASM_IS_LIST (tree_model), FALSE);

  disasm_list = DISASM_LIST(tree_model);

  iter->stamp = disasm_list->stamp;
  iter->user_data = child->user_data;
  iter->user_data2 = (uint32*)-1;

  return TRUE;
}


/*****************************************************************************
 *
 *  disasm_list_new:  This is what you use in your own code to create a
 *                    new Disasm list tree model for you to use.
 *
 *****************************************************************************/

DisasmList *
disasm_list_new (void)
{
  DisasmList *newDisasmlist;

  newDisasmlist = (DisasmList*) g_object_new (TYPE_DISASM_LIST, NULL);

  g_assert( newDisasmlist != NULL );

  return newDisasmlist;
}



void disasm_list_update (GtkTreeModel *tree_model, guint address)
{
  DisasmList    *disasm_list;

  if(!(DISASM_IS_LIST(tree_model)))
    return;

  disasm_list = DISASM_LIST(tree_model);
  if(address<PRELINES*4)
    address=PRELINES*4;

  if(address>(0xFFFFFFFC-(POSTLINES*4)))

    address=0xFFFFFFFC-(POSTLINES*4);
  disasm_list->startAddr = address;
}

