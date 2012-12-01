/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - desasm.c                                                *
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

#include <stdio.h>
#include <math.h>

#include <SDL.h>
#include <SDL_thread.h>

#include "desasm.h"
#include "ui_disasm_list.h"

//TODO: Lots and lots
// to differanciate between update (need reload) and scroll (doesn't need reload)
// to reorganise whole code.

#define DOUBLESCROLL
#define SCROLLRANGE    1.0f
#define SCROLLSLOW     0.1f
#define SCROLLSLOWAMT  ((int)(100/SCROLLSLOW))
 
static uint32 previous_focus;
static uint32 currentPC = 0;

static GtkWidget *clDesasm, *buRun;
static DisasmList *cmDesasm;
static GtkAdjustment *ajDesasm, *ajLinear, *ajLogari;

static void update_log_scroll(void);

static GdkColor color_normal, color_BP, color_PC, color_PC_on_BP;

// Callback functions
static void on_click( GtkTreeView *widget, GtkTreePath *path, 
              GtkTreeViewColumn *col, gpointer user_data );
static void on_scroll(GtkAdjustment *adjustment, gpointer user_data);

#ifdef DOUBLESCROLL
static void on_linear_scroll(GtkAdjustment *adjustment, gpointer user_data);
#endif //DOUBLESCROLL
static gboolean on_scroll_wheel(GtkWidget *widget, GdkEventScroll *event, 
                            gpointer user_data);

static gboolean on_scrollclick(GtkWidget *bar, GdkEventButton*, gpointer user_data);
static gboolean on_scrollrelease(GtkWidget *bar, GdkEventButton*, gpointer user_data);

static void on_step();
static void on_run();
static void on_goto();

static void on_close();

void disasm_set_color (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                       GtkTreeModel *tree_model, GtkTreeIter *iter,gpointer data)
{
  if ((long) iter->user_data2 == -1 && PC != NULL)
    {
      if(check_breakpoints((long) iter->user_data) != -1)
    {
      if(currentPC == ((long) iter->user_data))
        g_object_set(G_OBJECT(cell), "cell-background-gdk", 
             &color_PC_on_BP, "cell-background-set", TRUE, NULL);
      else
        g_object_set(G_OBJECT(cell), "cell-background-gdk", &color_BP,
             "cell-background-set", TRUE, NULL);
    }
      else
    {
      if(currentPC == ((long) iter->user_data))
        g_object_set(G_OBJECT(cell), "cell-background-gdk", &color_PC,
             "cell-background-set", TRUE, NULL);
      else
        g_object_set(G_OBJECT(cell), "cell-background-set", FALSE, NULL);
    }
    }
  else
    g_object_set(G_OBJECT(cell), "cell-background-set", FALSE, NULL);

  return;
}


//]}=-=-=-=-=-=-=-=-=-=-=[ Initialisation Desassembleur ]=-=-=-=-=-=-=-=-=-=-={[

void init_desasm()
{
    char title[64];

    GtkWidget 
      *boxH1,
      *scrollbar1,
      *boxV1,
      *buStep,
      *buGoTo,
      *swDesasm;

    desasm_opened = 1;
    
    winDesasm = gtk_window_new( GTK_WINDOW_TOPLEVEL );

    sprintf( title, "%s - %s", "Debugger", DEBUGGER_VERSION );
    gtk_window_set_title( GTK_WINDOW(winDesasm), title );
    gtk_window_set_default_size( GTK_WINDOW(winDesasm), 380, 500 );
    gtk_container_set_border_width( GTK_CONTAINER(winDesasm), 2 );

    gtk_window_set_deletable( GTK_WINDOW(winDesasm), FALSE );

    boxH1 = gtk_hbox_new( FALSE, 0 );

    gtk_container_add( GTK_CONTAINER(winDesasm), boxH1 );

    //=== Creation of the Disassembled Code Display ===/
    cmDesasm = disasm_list_new();

    clDesasm = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cmDesasm));
    g_object_unref(cmDesasm);

    gtk_tree_view_set_fixed_height_mode((GtkTreeView *) clDesasm, TRUE);

    GtkCellRenderer    *renderer;
    GtkTreeViewColumn  *col;
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Address", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_cell_data_func(col, renderer, disasm_set_color, NULL, NULL);

    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 100);
    gtk_tree_view_append_column( GTK_TREE_VIEW( clDesasm ), col);
    col = gtk_tree_view_column_new_with_attributes("Opcode", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 72);
    gtk_tree_view_append_column( GTK_TREE_VIEW( clDesasm ), col);
    col = gtk_tree_view_column_new_with_attributes("Args", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 140);
    gtk_tree_view_append_column( GTK_TREE_VIEW( clDesasm ), col);

    ajLogari = (GtkAdjustment *) gtk_adjustment_new(0, 0, SCROLLRANGE, 0.01, 0.1, 0.0);

    swDesasm = gtk_scrolled_window_new(NULL, ajLogari);

    gtk_scrolled_window_set_policy((GtkScrolledWindow *) swDesasm, GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    gtk_container_add((GtkContainer *) swDesasm, clDesasm);
    //This replaces clDesasm's adjustments with swDesasm's, so...
    ajDesasm = (GtkAdjustment *) gtk_adjustment_new(0, 0, 1, 1, 0.1, 0.1);

    //we replace it's vertical adjustment, with our own
    gtk_widget_set_scroll_adjustments(clDesasm, gtk_range_get_adjustment((GtkRange *)((GtkScrolledWindow *)swDesasm)->hscrollbar), ajDesasm);

    
    //gtk_container_add(boxH1, swDesasm);

#ifdef DOUBLESCROLL    
    ajLinear = (GtkAdjustment *) gtk_adjustment_new(0,0,((float)0xFFFFFFFF),0x4,0x60,0);

    scrollbar1 = gtk_vscrollbar_new( GTK_ADJUSTMENT(ajLinear));
    //gtk_container_add(boxH1, scrollbar1);
    gtk_box_pack_start( GTK_BOX(boxH1), scrollbar1, FALSE, TRUE, 0 );
#endif //DOUBLESCROLL

    gtk_box_pack_start( GTK_BOX(boxH1), swDesasm, TRUE, TRUE, 0 );

    // (doubles) value, lower, upper, step_increment, page_increment, page_size.

    //=== Creation of the Buttons =====================/
    boxV1 = gtk_vbox_new( FALSE, 2 );
    gtk_box_pack_end( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0 );
    
    //buRun = gtk_button_new_with_label( "Run" );
    buRun = gtk_button_new_with_label( "> \\ ||" );
    gtk_box_pack_start( GTK_BOX(boxV1), buRun, FALSE, FALSE, 5 );
    buStep = gtk_button_new_with_label( "Next" );
    gtk_box_pack_start( GTK_BOX(boxV1), buStep, FALSE, FALSE, 0 );
    buGoTo = gtk_button_new_with_label( "Go To..." );
    gtk_box_pack_start( GTK_BOX(boxV1), buGoTo, FALSE, FALSE, 20 );

    gtk_widget_show_all( winDesasm );

    gdk_window_set_events( winDesasm->window, GDK_ALL_EVENTS_MASK );

    //=== Signal Connection ===========================/
    gtk_signal_connect( GTK_OBJECT(clDesasm), "row-activated",
                    GTK_SIGNAL_FUNC(on_click), NULL );
    gtk_signal_connect( GTK_OBJECT(ajLogari), "value-changed",
                    GTK_SIGNAL_FUNC(on_scroll), NULL );
#ifdef DOUBLESCROLL
    gtk_signal_connect( GTK_OBJECT(ajLinear), "value-changed",
            GTK_SIGNAL_FUNC(on_linear_scroll), NULL );
#endif
    gtk_signal_connect( GTK_OBJECT(winDesasm), "scroll-event", GTK_SIGNAL_FUNC(on_scroll_wheel), NULL );
    gtk_signal_connect( GTK_OBJECT(swDesasm), "scroll-event", GTK_SIGNAL_FUNC(on_scroll_wheel), NULL );
    gtk_signal_connect( GTK_OBJECT(scrollbar1), "scroll-event", GTK_SIGNAL_FUNC(on_scroll_wheel), NULL );
    gtk_signal_connect( GTK_OBJECT(gtk_scrolled_window_get_vscrollbar(
            GTK_SCROLLED_WINDOW(swDesasm))), "scroll-event", GTK_SIGNAL_FUNC(on_scroll_wheel), NULL );

    gtk_signal_connect( GTK_OBJECT(gtk_scrolled_window_get_vscrollbar((GtkScrolledWindow *) swDesasm)), "button-press-event", GTK_SIGNAL_FUNC(on_scrollclick), NULL);
    gtk_signal_connect( GTK_OBJECT(gtk_scrolled_window_get_vscrollbar((GtkScrolledWindow *) swDesasm)), "button-release-event", GTK_SIGNAL_FUNC(on_scrollrelease), NULL);
    gtk_signal_connect( GTK_OBJECT(buRun), "clicked", on_run, NULL );
    gtk_signal_connect( GTK_OBJECT(buStep), "clicked", on_step, NULL );
    gtk_signal_connect( GTK_OBJECT(buGoTo), "clicked", on_goto, NULL );
    gtk_signal_connect( GTK_OBJECT(winDesasm), "destroy", on_close, NULL );

    //=== Colors Initialisation =======================/
//je l'aurais bien fait en global, mais ca entrainait des erreurs. Explication?
    color_normal.red = 0xFFFF;
    color_normal.green = 0xFFFF;
    color_normal.blue = 0xFFFF;

    color_BP.red=0xFFFF;
    color_BP.green=0xFA00;
    color_BP.blue=0x0;

    color_PC.red=0x0;
    color_PC.green=0xA000;
    color_PC.blue=0xFFFF;

    color_PC_on_BP.red=0xFFFF;
    color_PC_on_BP.green=0x0;
    color_PC_on_BP.blue=0x0;

    previous_focus = 0x00000000;
}


//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Desassembleur ]=-=-=-=-=-=-=-=-=-=-=[
unsigned int addtest=0x00;
unsigned int prevadd=0x00;
unsigned int mousedown=0x00;
float prev=0.0f;

void update_disassembler( uint32 pc )
{
    currentPC=pc;
    update_desasm( pc );
}


void update_desasm( uint32 focused_address )
//Display disassembled instructions around a 'focused_address'
//  (8 instructions before 'focused_address', the rest after).
{
    addtest=focused_address;
    disasm_list_update((GtkTreeModel *) cmDesasm, focused_address);
    
    gtk_adjustment_set_value(ajLinear, ((float)addtest));
    
    if(mousedown==0)
      update_log_scroll();
    
    refresh_desasm();
    previous_focus = focused_address;
}


void refresh_desasm()
{
  GtkTreePath *path,*path2;
    GtkTreeIter iter;

    int i;
    i=(((int)(addtest/4))-((int)(previous_focus/4)));
    
    path = gtk_tree_path_new_from_string("0");
    path2 = gtk_tree_path_new_from_string("26");
    if(i>0)
      {
    if (i<400)
      for(;i>0; i--)
        {
          gtk_tree_model_get_iter((GtkTreeModel *) cmDesasm, &iter, path2);
    
          gtk_tree_model_row_inserted((GtkTreeModel *) cmDesasm, path2, &iter);
          gtk_tree_model_row_deleted((GtkTreeModel *) cmDesasm, path);
          //          gtk_tree_path_next(path2);

        }
      }
    else if(i<0)
      {
    if(i>-400)
      for(;i<0; i++)
        {
          gtk_tree_model_get_iter((GtkTreeModel *) cmDesasm, &iter, path);
    
          gtk_tree_model_row_inserted((GtkTreeModel *) cmDesasm, path, &iter);
          gtk_tree_model_row_deleted((GtkTreeModel *) cmDesasm, path2);
          //          gtk_tree_path_next(path2);

        }
      
      }
    
    for(i=0; i<32; i++)
      {
    gtk_tree_model_get_iter((GtkTreeModel *) cmDesasm, &iter, path);
    
    //gtk_tree_model_row_deleted(cmDesasm, path);
    //gtk_tree_model_row_inserted(cmDesasm, path, &iter);
    gtk_tree_model_row_has_child_toggled((GtkTreeModel *) cmDesasm, path, &iter);
    gtk_tree_path_next(path);
      }

    gtk_tree_path_free(path);
    gtk_tree_path_free(path2);
    gtk_widget_queue_draw( clDesasm );
}


void switch_button_to_run()
{ //Is called from debugger.c, when a breakpoint is reached.
    //gtk_label_set_text( GTK_LABEL (GTK_BIN (buRun)->child), "Run");
    //todo: this causes a deadlock or something the second time a
    //breakpoint hits, breaking the interface. The other lines changing
    //this label have been commented with the note "avoid deadlock".
}


//]=-=-=-=-=-=-=[ Les Fonctions de Retour des Signaux (CallBack) ]=-=-=-=-=-=-=[

static void on_run()
{
    if(run == 2) {
        run = 0;
        //gtk_label_set_text( GTK_LABEL (GTK_BIN (buRun)->child), "Run"); //avoid deadlock
    } else {
        run = 2;
        //gtk_label_set_text( GTK_LABEL (GTK_BIN (buRun)->child),"Pause"); //avoid deadlock
        debugger_step();
    }
}


static void on_step()
{
    if(run == 2) {
        //gtk_label_set_text( GTK_LABEL (GTK_BIN (buRun)->child), "Run"); //avoid deadlock
    } else {
        debugger_step();
    }
    run = 0;
}


static void on_goto()
{//TODO: to open a dialog, get & check the entry, and go there.
    update_desasm( 0x0A4000040 );
}

static void update_log_scroll()
{
  if(mousedown==0) {
    if(addtest<SCROLLSLOWAMT*0.5f)
      gtk_adjustment_set_value(ajLogari, (addtest/((float)SCROLLSLOWAMT)));

    else if(addtest>0xFFFFFFFF-(SCROLLSLOWAMT*0.5f))
      gtk_adjustment_set_value(ajLogari, 1.0f - ((0xFFFFFFFF-addtest)/((float)SCROLLSLOWAMT)));
    else
      gtk_adjustment_set_value(ajLogari, 0.5f);
  }
}

#ifdef DOUBLESCROLL
static void on_linear_scroll(GtkAdjustment *adjustment, gpointer usr)
{
  if(mousedown==0) {
    addtest=adjustment->value;
    if(adjustment->value - ((double)addtest) > 0x80000000)
      addtest=0xFFFFFFFC;
    addtest=addtest&0xFFFFFFFC;
  }

  update_desasm( (uint32) addtest );
}
#endif

static gboolean on_scroll_wheel(GtkWidget *widget, GdkEventScroll *event,
                            gpointer user_data)
{
  if(event->direction==GDK_SCROLL_UP)        addtest-=4;
  else if(event->direction==GDK_SCROLL_DOWN) addtest+=4;
  else return FALSE;

  update_desasm( (uint32) addtest );
  return TRUE;
}

static void on_scroll(GtkAdjustment *adjustment, gpointer user_data)
{
  if(mousedown==0)
    {
      update_log_scroll();
      return;
    }
  float delta = adjustment->value - prev;

  int ex;
  float base, scale;
  double flttest;

  if(delta>0.0f)
    {
      base = frexp((float)((0xffffffff)-prevadd-((1.0-prev)*SCROLLSLOWAMT))+2.0f,&ex);
      scale = 1.0f/(1.0f-prev-SCROLLSLOW);
      if(scale<0.0f)
    scale=0.0f;
      flttest=((double)prevadd) + ((double)SCROLLSLOWAMT) * delta + (base * pow(2.0f, ex*(delta-SCROLLSLOW)*scale));
      addtest=flttest;
    }
  else
    {
      base = frexp((float)(prevadd-(prev*SCROLLSLOWAMT))+2.0f,&ex);
      scale = 1.0f/(prev-SCROLLSLOW);
      if(scale<0.0f)
    scale=0.0f;
      flttest=((double)prevadd) + ((double)SCROLLSLOWAMT) * delta - (base * pow(2.0f, ex*(-delta-SCROLLSLOW)*scale));
      if(flttest>0.0f)
    addtest=flttest;
      else
    addtest=0;
    }

  if((adjustment->value)==1.0f)
    addtest=0xFFFFFFFC;

  addtest=addtest&0xFFFFFFFC;

  update_desasm( (uint32) addtest);

  //printf("scroll %08x %f %f %f %f %d\n", addtest, delta, flttest, base, scale, ex);
    

}

static gboolean on_scrollclick(GtkWidget *bar, GdkEventButton* evt, gpointer user_data)
{
  prev=(gtk_range_get_adjustment((GtkRange *) bar)->value);
  prevadd=addtest;
  mousedown=1;
  return FALSE;
}

static gboolean on_scrollrelease(GtkWidget *bar, GdkEventButton* evt, gpointer user_data)
{
  //printf("prevadd %08x  prev %f\n", prevadd,prev);
  mousedown=0;
  gtk_adjustment_set_value(gtk_range_get_adjustment((GtkRange *) bar), 0.5f);
  return FALSE;
}

static void on_click( GtkTreeView *widget, GtkTreePath *path, 
               GtkTreeViewColumn *col, gpointer user_data )
//Add a breakpoint on double-clicked linelines.
{
  uint32 clicked_address;
  int break_number;
  GtkTreeIter iter;

  if(gtk_tree_path_get_depth(path) > 1)
    return;// do nothing if it is recompiler disassembly

  disasm_list_get_iter((GtkTreeModel *) cmDesasm, &iter, path);

  clicked_address =(uint32)(long) iter.user_data;

  break_number = lookup_breakpoint(clicked_address, 0, BPT_FLAG_EXEC);

  if( break_number==-1 ) {
    add_breakpoint( clicked_address );
  }
  else if(BPT_CHECK_FLAG(g_Breakpoints[break_number], BPT_FLAG_ENABLED)) {
    disable_breakpoint( break_number );
  }
  else {
    enable_breakpoint( break_number );
  }

  update_breakpoints();
  refresh_desasm();
}

static void on_close()
{
  //    desasm_opened = 0;

//  What should be happening then? Currently, thread is killed.
//    run = 0;            // 0(stop) ou 2(run)

  //For now, lets get rid of the close button, and keep this window always open
  //for the execution controls to be accessible
}

