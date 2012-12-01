/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - memedit.c                                               *
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

#include <math.h>

#include <gdk/gdkkeysyms.h>

#include "memedit.h"

#include "../../main.h"
#include "../../translate.h"

GtkTextBuffer *textbuf;
GtkTextTag *textfont;

//note: just changing these constants is not (yet) enough to properly change the display.
const uint32 num_menu_items = 6;
const int bytesperline = 16;
const int hexstartpos = 9;
//apparently the compiler isn't smart enough to figure out that these equations are constant. -_-
const int linelen = 74; //hexstartpos + (bytesperline * 3) + bytesperline + 1; //address, separator, hex, text, line break (hex/text separator included in hex)
const int textstartpos = 57; //hexstartpos + (bytesperline * 3); //address, separator, hex
uint32 memedit_address, memedit_numlines;
GtkWidget *menu;
GtkWidget* menuitem[6]; //DO NOT FORGET TO UPDATE THIS WHEN YOU ADD ITEMS
GtkClipboard* clipboard;


static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_mouse_down(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void on_menu_select(GtkMenuItem *menuitem, gpointer user_data);
static void on_auto_refresh_toggle(GtkToggleButton *togglebutton, gpointer user_data);
static void on_close();

void update_memory_editor()
{
    int i, j;
    char line[linelen + 1];
    char* buf;
    uint32 addr;
    uint8 byte[bytesperline];
    GtkTextIter textstart, textend;
    
    buf = (char*)malloc(memedit_numlines * sizeof(line) * sizeof(char));
    if(!buf)
    {
        printf("update_memory_editor: out of memory at %s:%u\n", __FILE__, __LINE__);
        return;
    }

    
    //Read memory
    //todo: display 'XX' or something for unreadable memory,
    //maybe colour the text
    addr = memedit_address;
    buf[0] = 0;
    for(i=0; i<memedit_numlines; i++)
    {
        for(j=0; j<bytesperline; j++)
            byte[j] = read_memory_8(addr + j);
        
        sprintf(line, "%08X|%02X %02X %02X %02X|%02X %02X %02X %02X|%02X %02X %02X %02X|%02X %02X %02X %02X|",
            addr, byte[0], byte[1], byte[2], byte[3], byte[4], byte[5], byte[6], byte[7],
            byte[8], byte[9], byte[10], byte[11], byte[12], byte[13], byte[14], byte[15]);
        
        strcat(buf, line);
        for(j=0; j<bytesperline; j++)
        {
            if((byte[j] >= 0x20) && (byte[j] <= 0x7E))
                line[j] = byte[j];
            else line[j] = '.';
        }
        
        if(i < (memedit_numlines - 1))
        {
            line[bytesperline] = '\n';
            line[bytesperline + 1] = 0;
        }
        else line[bytesperline] = 0;
        strcat(buf, line);
        addr += bytesperline;
    }
    
    //Update textbox
    gtk_text_buffer_set_text(textbuf, buf, -1);
    gtk_text_buffer_get_bounds(textbuf, &textstart, &textend); //todo: there must be a better way to keep it in monospace
    gtk_text_buffer_apply_tag(textbuf, textfont, &textstart, &textend);
    
    free(buf);
}

//]=-=-=-=-=-=-=-=-=-=-=-=[ Memory Editor Initialisation ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_memedit()
{
    int i;
    GtkWidget *textbox, *hbox, *vbox, *buAutoRefresh;
    
    memedit_address = 0x80000000;
    memedit_numlines = 16;
    memedit_auto_update = 0;

    //=== Creation of Memory Editor ===========/
    //Create the window.
    winMemEdit = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW(winMemEdit), "Memory");
    gtk_window_set_default_size( GTK_WINDOW(winMemEdit), 500, 200);
    gtk_container_set_border_width( GTK_CONTAINER(winMemEdit), 2);
    
    //Create the containers.
    hbox = gtk_hbox_new(FALSE, 0); //hbox holds the buttons at the bottom.
    vbox = gtk_vbox_new(FALSE, 0); //vbox holds the textbox and hbox.
    
    //Create the textbox.
    textbox = gtk_text_view_new();
    textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textbox));
    
    textfont = gtk_text_buffer_create_tag(textbuf, "font", "font", "monospace", NULL);
    update_memory_editor();
    
    //Add textbox to container.
    gtk_box_pack_start(GTK_BOX(vbox), textbox, TRUE, TRUE, 0);
    
    //Create buttons and add to container.
    buAutoRefresh = gtk_check_button_new_with_label("Auto Refresh");
    gtk_box_pack_start(GTK_BOX(hbox), buAutoRefresh, FALSE, FALSE, 0);
    
    //Add container to window.
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(winMemEdit), vbox);
    
    //Get handle to clipboard.
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    
    //Create right-click menu
    menu = gtk_menu_new();
    menuitem[0] = gtk_menu_item_new_with_mnemonic("_Copy");
    menuitem[1] = gtk_menu_item_new_with_mnemonic("Copy as _Binary");
    menuitem[2] = gtk_menu_item_new_with_mnemonic("Copy as _ASCII");
    menuitem[3] = gtk_menu_item_new_with_mnemonic("Add _Read Breakpoint");
    menuitem[4] = gtk_menu_item_new_with_mnemonic("Add _Write Breakpoint");
    menuitem[5] = gtk_menu_item_new_with_mnemonic("_Disable Breakpoints");
    
    for(i=0; i<num_menu_items; i++)
    {
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem[i]);
        gtk_widget_show(menuitem[i]);
    }
    
    gtk_widget_show_all(winMemEdit);
    gtk_widget_show(menu);

    //=== Signal Connections ===========================/
    gtk_signal_connect( GTK_OBJECT(winMemEdit), "destroy", (GtkSignalFunc)on_close, NULL );
    gtk_signal_connect( GTK_OBJECT(textbox), "key-press-event", (GtkSignalFunc)on_key_press, NULL );
    gtk_signal_connect( GTK_OBJECT(textbox), "button-press-event", (GtkSignalFunc)on_mouse_down, NULL );
    gtk_signal_connect(GTK_OBJECT(buAutoRefresh), "clicked", (GtkSignalFunc)on_auto_refresh_toggle, NULL);
    
    for(i=0; i<num_menu_items; i++)
        gtk_signal_connect( GTK_OBJECT(menuitem[i]), "activate", (GtkSignalFunc)on_menu_select, (gpointer)(long)i);
        
    memedit_opened = 1;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    GtkTextMark* cursor;
    GtkTextIter cursor_iter;
    int cursorpos, linepos, linenum;
    uint32 key, addr;
    uint8 byte = 0;
    
    //Figure out what was typed.
    //todo: make this not suck
    switch(event->keyval)
    {
        case GDK_0: case GDK_1: case GDK_2:
        case GDK_3: case GDK_4: case GDK_5:
        case GDK_6: case GDK_7: case GDK_8:
        case GDK_9:
            key = event->keyval - GDK_0;
        break;
        
        case GDK_KP_0: case GDK_KP_1: case GDK_KP_2:
        case GDK_KP_3: case GDK_KP_4: case GDK_KP_5:
        case GDK_KP_6: case GDK_KP_7: case GDK_KP_8:
        case GDK_KP_9:
            key = event->keyval - GDK_KP_0;
        break;
        
        case GDK_A: case GDK_B: case GDK_C:
        case GDK_D: case GDK_E: case GDK_F:
            key = (event->keyval - GDK_A) + 10;
        break;
        
        case GDK_a: case GDK_b: case GDK_c:
        case GDK_d: case GDK_e: case GDK_f:
            key = (event->keyval - GDK_a) + 10;
        break;
        
        default:
            key = event->keyval;
        break;
    }
    
    //Get the cursor position.
    cursor = gtk_text_buffer_get_insert(textbuf);
    gtk_text_buffer_get_iter_at_mark(textbuf, &cursor_iter, cursor);
    cursorpos = gtk_text_iter_get_offset(&cursor_iter);
    
    
    //React to the keypress.
    //todo: skip between-bytes and separator areas when
    //navigating with arrow keys or mouse.
    if(key == GDK_Up)
    {
        cursorpos -= linelen;
        if(cursorpos < 0)
        {
            memedit_address -= bytesperline;
            cursorpos += linelen;
        }
    }
    else if(key == GDK_Down)
    {
        cursorpos += linelen;
        if(cursorpos >= (linelen * memedit_numlines))
        {
            memedit_address += bytesperline;
            cursorpos -= linelen;
        }
    }
    else if(key == GDK_Left)
    {
        if(cursorpos) cursorpos--;
    }
    else if(key == GDK_Right)
    {
        if(cursorpos < (linelen * memedit_numlines)) cursorpos++;
    }
    else
    {
        //Determine where we are.
        linenum = cursorpos / linelen;
        linepos = cursorpos % linelen;
        
        if((event->keyval != GDK_BackSpace) && ((linepos == (hexstartpos - 1)) || (linepos == (textstartpos - 1)))) //address/hex or hex/text separators
        {
            linepos++;
            cursorpos++;
        }
        
        //printf("line %u, pos %u: ", linenum, linepos);
        if(linepos >= (linelen - 1)); //end of line; do nothing
        
        //If cursor is in address
        else if(linepos < (hexstartpos - 1))
        {
            if(key < 16) //hex number entered
            {
                //yes, I probably _could_ have made this line uglier.
                memedit_address = (memedit_address & ~(0xF << (4 * (7 - linepos)))) | (key << (4 * (7 - linepos)));
                cursorpos++;
                if((cursorpos % linelen) == (hexstartpos - 1)) cursorpos++; //skip address/hex separator
            }
            else if(linepos && (event->keyval == GDK_BackSpace)) //back one character
            {
                cursorpos--;
                if(cursorpos < 0)
                {
                    memedit_address -= bytesperline;
                    cursorpos++;
                }
            }
            //todo: else, beep or something
        }
        
        //If cursor is in text
        else if(linepos >= (textstartpos - 1))
        {
            //If a non-special key, except Enter, was pressed
            if((event->keyval <= 0xFF) || (event->keyval == GDK_Return) || (event->keyval == GDK_KP_Enter))
            {
                linepos -= textstartpos; //Character index
                addr = memedit_address + (linenum * bytesperline) + linepos; //Address to edit
                
                if(event->keyval <= 0xFF) byte = event->keyval;
                else if((event->keyval == GDK_Return) || (event->keyval == GDK_KP_Enter)) byte = 0x0A; //Enter inserts line break
                
                write_memory_8(addr, byte);
                cursorpos++;
                if((cursorpos % linelen) == (linelen - 1)) //wrote past last character
                {
                    cursorpos += (textstartpos + 1); //to first char on next line (+1 for line break)
                    if(cursorpos >= (linelen * memedit_numlines)) //past end of box
                    {
                        memedit_address += bytesperline;
                        cursorpos -= linelen;
                    }
                }
            }
            else if(event->keyval == GDK_BackSpace) //back one character
            {
                cursorpos--;
                if((cursorpos % linelen) < textstartpos) //before first character
                {
                    cursorpos -= (textstartpos + 1); //to last char on prev line
                    if(cursorpos < 0)
                    {
                        memedit_address -= bytesperline;
                        cursorpos += linelen;
                    }
                }
            }
        }
        
        //If cursor is in hex
        else
        {
            linepos -= (hexstartpos - 1);
            if((event->keyval != GDK_BackSpace) && !(linepos % 3)) //between bytes
            {
                cursorpos++;
                linepos++;
            }
            
            addr = memedit_address + (linenum * bytesperline) + (linepos / 3);
            
            //printf("byte %u (%08X) nybble %u, ", linepos / 3, addr, (linepos % 3) - 1);
            if(key < 16) //hex number entered
            {
                byte = read_memory_8(addr);
                //printf("%02X -> ", byte);
                if((linepos % 3) - 1) //low nybble
                {
                    byte = (byte & 0xF0) | (uint8)key;
                    cursorpos++; //skip the between-bytes area
                }
                else byte = (byte & 0x0F) | ((uint8)key << 4);
                //printf("%02X\n", byte);
                write_memory_8(addr, byte);
                cursorpos++;
                
                if((cursorpos % linelen) == textstartpos) //wrote past last byte
                {
                    cursorpos += bytesperline + 10; //to first byte on next line - skip text (bytesperline chars), line break, address (8 chars), separator
                    if(cursorpos >= (linelen * memedit_numlines)) //past end of box
                    {
                        memedit_address += 16;
                        cursorpos -= linelen;
                    }
                }
            } //if hex number entered
            else if(event->keyval == GDK_BackSpace) //back one character
            {
                if(linepos < 2) cursorpos -= linepos + 1; //end of address (before/after separator)
                else
                {
                    cursorpos--;
                    if(!(((cursorpos % linelen) - (hexstartpos - 1)) % 3)) cursorpos--; //between bytes
                    
                    if((cursorpos % linelen) < hexstartpos) //before first character
                    {
                        cursorpos -= (hexstartpos + bytesperline + 1); //to last char on prev line
                        if(cursorpos < 0)
                        {
                            memedit_address -= bytesperline;
                            cursorpos += linelen;
                        }
                    }
                }
            } //if backspace
        } //if in hex
    }
    
    //0.........1.........2.........3.........4.........5.........6.........7..
    //|00 01 02 03|04 05 06 07|08 09 0A 0B|0C 0D 0E 0F|0123456789ABCDEF
    //80000000|00 01 02 03|04 05 06 07|08 09 0A 0B|0C 0D 0E 0F|0123456789ABCDEF
    
    update_memory_editor();
    
    //Restore the cursor position.
    gtk_text_buffer_get_iter_at_offset(textbuf, &cursor_iter, cursorpos);
    gtk_text_buffer_place_cursor(textbuf, &cursor_iter);
    
    return TRUE;
}


static gboolean on_mouse_down(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if(event->button != 3) return FALSE;
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, 0, event->button, event->time);
    return TRUE;
}


static void on_menu_select(GtkMenuItem *menuitem, gpointer user_data)
{
    gchar* text;
    GtkTextIter start, end;
    uint32 startaddr, endaddr = 0, numbytes;
    uint8 byte;
    int i;
    breakpoint newbp;
    
    switch((long)user_data)
    {
        case 0: //Copy
            if(!gtk_text_buffer_get_selection_bounds(textbuf, &start, &end)) break; //nothing selected if false
            text = gtk_text_iter_get_text(&start, &end);
            gtk_clipboard_set_text(clipboard, text, -1);
        break;
        
        case 1: //Copy as Binary
            //TODO - not sure how to put raw binary data on clipboard <_<
            error_message(tr("not implemented yet"));
        break;
        
        case 2: //Copy as ASCII
            numbytes = GetMemEditSelectionRange(&startaddr, 0);
            if(!numbytes) break;
            text = (gchar*)malloc((numbytes + 1) * sizeof(gchar));
            if(!text) break;
            
            for(i=0; i<numbytes; i++)
            {
                byte = read_memory_8(startaddr);
                if((byte >= 0x20) && (byte <= 0x7E)) text[i] = byte;
                else text[i] = '.';
                startaddr++;
            }
            text[i] = 0;
            
            gtk_clipboard_set_text(clipboard, text, i);
            free(text);
        break;
        
        case 3: //Break on read
        case 4: //Break on write
            numbytes = GetMemEditSelectionRange(&startaddr, 1);
            if(!numbytes) break;
            endaddr = startaddr + (numbytes - 1);
            
            //see if there's already a breakpoint for this range,
            //and if so, just add the read/write flag.
            for(i=0; i<g_NumBreakpoints; i++)
            {
                if((g_Breakpoints[i].address == startaddr)
                && (g_Breakpoints[i].endaddr == endaddr))
                {
                    g_Breakpoints[i].flags |= ((long)user_data == 3) ? BPT_FLAG_READ : BPT_FLAG_WRITE;
                    printf("Added %s flag to breakpoint %d.\n", ((long)user_data == 3) ? "read" : "write", i);
                    update_breakpoints();
                    numbytes = 0;
                    break;
                }
            }
            
            if(!numbytes) break; //already found a breakpoint
            
            newbp.address = startaddr;
            newbp.endaddr = endaddr;
            newbp.flags = BPT_FLAG_ENABLED | (((long)user_data == 3) ? BPT_FLAG_READ : BPT_FLAG_WRITE);
            add_breakpoint_struct(&newbp);
            printf("Added breakpoint.\n");
            update_breakpoints();
        break;
        
        case 5: //Clear breakpoints
            numbytes = GetMemEditSelectionRange(&startaddr, 1);
            if(!numbytes)
            {
                //todo: prompt to clear all memory breakpoints
                break;
            }
            
            for(i=0; i<g_NumBreakpoints; i++)
            {
                //if breakpoint overlaps this range
                if(((g_Breakpoints[i].address <= startaddr) && (g_Breakpoints[i].endaddr >= startaddr))
                || ((g_Breakpoints[i].address <= endaddr) && (g_Breakpoints[i].endaddr >= endaddr)))
                {
                    g_Breakpoints[i].flags &= ~BPT_FLAG_ENABLED;
                    printf("Disabled breakpoint %d.\n", i);
                }
            }
            update_breakpoints();
        break;
    }
}


//Auto-refresh checkbox handler.
static void on_auto_refresh_toggle(GtkToggleButton *togglebutton, gpointer user_data)
{
    memedit_auto_update = gtk_toggle_button_get_active(togglebutton) ? 1 : 0;
}


static void on_close()
{
    memedit_opened = 0;
}


//Determines range of bytes selected.
//Returns # of bytes selected, sets StartAddr to address of first byte if any.
//A byte is considered selected if the selection covers either the hex OR text view of it.
//In the hex view, a byte is considered selected if either nybble and/or the space/pipe
//after it is selected. I consider this a good thing, as it means if you accidentally don't
//select an entire byte, it still gets copied.
//When AllowEmpty is set, if there is no selection, but the cursor is in the hex or text,
//the byte next to it is considered selected.
int GetMemEditSelectionRange(uint32* StartAddr, int AllowEmpty)
{
    GtkTextIter start, end;
    GtkTextMark* cursor;
    int startpos, endpos, startline, endline, startlinepos, endlinepos, numbytes;
    
    if(!memedit_opened) return 0;
    
    //Determine what byte range is selected.
    if(gtk_text_buffer_get_selection_bounds(textbuf, &start, &end))
    {
        startpos = gtk_text_iter_get_offset(&start);
        endpos = gtk_text_iter_get_offset(&end);
    }
    else //nothing selected
    {
        if(!AllowEmpty) return 0;
        cursor = gtk_text_buffer_get_insert(textbuf);
        gtk_text_buffer_get_iter_at_mark(textbuf, &start, cursor);  
        startpos = gtk_text_iter_get_offset(&start);
        endpos = startpos + 1;
    }
    //printf("selected %d - %d\n", startpos, endpos);
    
    startline = startpos / linelen;
    startlinepos = startpos % linelen;
    endline = endpos / linelen;
    endlinepos = endpos % linelen;
    
    if((startlinepos == (textstartpos - 1)) && ((endpos - startpos) == 1)) return 0; //Only hex/text separator selected
    
    if(startlinepos >= (linelen - 1)) //past end of line
    {
        startline++;
        startlinepos = hexstartpos;
    }
    
    if(endlinepos <= hexstartpos) //before hex
    {
        endline--;
        endlinepos = textstartpos - 1;
    }
    
    //Quiet, it works.
    if(startline > endline) return 0; //no actual hex/text selected
    //else if((startline == endline) && ((endlinepos - startlinepos) < 2) && (startlinepos < (textstartpos - 1))) return 0; //no entire bytes
    
    if(startlinepos >= textstartpos) //selection begins in text
        (*StartAddr) = (startline * bytesperline) + (startlinepos - textstartpos);
    else (*StartAddr) = (startline * bytesperline) + ((startlinepos - hexstartpos) / 3);
    
    if(endlinepos >= textstartpos) //selection ends in text
        numbytes = ((endline * bytesperline) + (endlinepos - textstartpos)) - (*StartAddr);
    else numbytes = ((endline * bytesperline) + (int)ceil((float)(endlinepos - hexstartpos) / 3.0)) - (*StartAddr);

    (*StartAddr) += memedit_address;
    //printf("%u bytes from %08X\n", numbytes, (*StartAddr));
    return numbytes;
}

