/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main.c                                                  *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
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

#include <SDL.h>
#include <string.h>

#include "../main/winlnxdefs.h"
#include "Controller_1.1.h"

#ifdef USE_GTK
#include <gtk/gtk.h>
#endif

static CONTROL_INFO control_info;
static BUTTONS b;
static int shift_state = 0;
static int ctrl_state = 0;
static int left_state = 0;
static int right_state = 0;
static int up_state = 0;
static int down_state = 0;

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL CloseDLL (void)
{
}

/******************************************************************
  Function: ControllerCommand
  Purpose:  To process the raw data that has just been sent to a 
            specific controller.
  input:    - Controller Number (0 to 3) and -1 signalling end of 
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none
  
  note:     This function is only needed if the DLL is allowing raw
            data, or the plugin is set to raw

            the data that is being processed looks like this:
            initilize controller: 01 03 00 FF FF FF 
            read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand ( int Control, BYTE * Command)
{
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllAbout ( HWND hParent )
{
   char s[] = "Input plugin for Mupen64 emulator\nMade by Hacktarux\n";
#ifdef USE_GTK
   GtkWidget *dialog, *label, *okay_button;
   
   /* Create the widgets */
   
   dialog = gtk_dialog_new();
   label = gtk_label_new (s);
   okay_button = gtk_button_new_with_label("OK");
   
   /* Ensure that the dialog box is destroyed when the user clicks ok. */
   
   gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
                  GTK_SIGNAL_FUNC (gtk_widget_destroy),
                  GTK_OBJECT (dialog));
   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
              okay_button);
   
   /* Add the label, and show everything we've added to the dialog. */
   
   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
              label);
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_widget_show_all (dialog);
#else
   printf(s);
#endif
}

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllConfig ( HWND hParent )
{
}

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllTest ( HWND hParent )
{
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
   PluginInfo->Version = 0x0101;
   PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
   strcpy(PluginInfo->Name, "Mupen64 basic input plugin");
}

/******************************************************************
  Function: GetKeys
  Purpose:  To get the current state of the controllers buttons.
  input:    - Controller Number (0 to 3)
            - A pointer to a BUTTONS structure to be filled with 
            the controller state.
  output:   none
*******************************************************************/    
EXPORT void CALL GetKeys(int Control, BUTTONS * Keys )
{
   if (Control == 0)
     Keys->Value = b.Value;
}

/******************************************************************
  Function: InitiateControllers
  Purpose:  This function initialises how each of the controllers 
            should be handled.
  input:    - The handle to the main window.
            - A controller structure that needs to be filled for 
              the emulator to know how to handle each controller.
  output:   none
*******************************************************************/  
EXPORT void CALL InitiateControllers (CONTROL_INFO ControlInfo)
{
   control_info = ControlInfo;
   control_info.Controls[0].Present = TRUE;
   control_info.Controls[0].Plugin = PLUGIN_MEMPAK;
}

/******************************************************************
  Function: ReadController
  Purpose:  To process the raw data in the pif ram that is about to
            be read.
  input:    - Controller Number (0 to 3) and -1 signalling end of 
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none  
  note:     This function is only needed if the DLL is allowing raw
            data.
*******************************************************************/
EXPORT void CALL ReadController ( int Control, BYTE * Command )
{
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomClosed (void)
{
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the 
            emulation thread)
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomOpen (void)
{
}

void update_analogic_state()
{
   if (ctrl_state)
     {
    if (shift_state)
      {
         if (left_state) b.Y_AXIS = -40;
         if (right_state) b.Y_AXIS = 40;
         if (up_state) b.X_AXIS = 40;
         if (down_state) b.X_AXIS = -40;
      }
    else
      {
         if (left_state) b.Y_AXIS = -20;
         if (right_state) b.Y_AXIS = 20;
         if (up_state) b.X_AXIS = 20;
         if (down_state) b.X_AXIS = -20;
      }
     }
   else
     {
    if (shift_state)
      {
         if (left_state) b.Y_AXIS = -60;
         if (right_state) b.Y_AXIS = 60;
         if (up_state) b.X_AXIS = 60;
         if (down_state) b.X_AXIS = -60;
      }
    else
      {
         if (left_state) b.Y_AXIS = -80;
         if (right_state) b.Y_AXIS = 80;
         if (up_state) b.X_AXIS = 80;
         if (down_state) b.X_AXIS = -80;
      }
     }
   if (left_state == right_state) b.Y_AXIS = 0;
   if (up_state == down_state) b.X_AXIS = 0;
}

/******************************************************************
  Function: WM_KeyDown
  Purpose:  To pass the WM_KeyDown message from the emulator to the 
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/ 
EXPORT void CALL WM_KeyDown( WPARAM wParam, LPARAM lParam )
{
   switch (lParam) // lParam = key symbol
     {
      case SDLK_KP6:
    b.R_DPAD = 1;
    break;
      case SDLK_KP4:
    b.L_DPAD = 1;
    break;
      case SDLK_KP2:
    b.D_DPAD = 1;
    break;
      case SDLK_KP8:
    b.U_DPAD = 1;
    break;
      case SDLK_RETURN:
    b.START_BUTTON = 1;
    break;
      case SDLK_SPACE:
    b.Z_TRIG = 1;
    break;
      case SDLK_x:
    b.B_BUTTON = 1;
    break;
      case SDLK_w:
    b.A_BUTTON = 1;
    break;
      case SDLK_PAGEDOWN:
    b.R_CBUTTON = 1;
    break;
      case SDLK_DELETE:
    b.L_CBUTTON = 1;
    break;
      case SDLK_END:
    b.D_CBUTTON = 1;
    break;
      case SDLK_HOME:
    b.U_CBUTTON = 1;
    break;
      case SDLK_s:
    b.R_TRIG = 1;
    break;
      case SDLK_q:
    b.L_TRIG = 1;
    break;
      case SDLK_LSHIFT:
    shift_state = 1;
    update_analogic_state();
    break;
      case SDLK_LCTRL:
    ctrl_state = 1;
    update_analogic_state();
    break;
      case SDLK_UP:
    up_state = 1;
    update_analogic_state();
    break;
      case SDLK_DOWN:
    down_state = 1;
    update_analogic_state();
    break;
      case SDLK_RIGHT:
    right_state = 1;
    update_analogic_state();
    break;
      case SDLK_LEFT:
    left_state = 1;
    update_analogic_state();
    break;
     }
}

/******************************************************************
  Function: WM_KeyUp
  Purpose:  To pass the WM_KEYUP message from the emulator to the 
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/ 
EXPORT void CALL WM_KeyUp( WPARAM wParam, LPARAM lParam )
{
   switch (lParam) // lParam = key symbol
     {
      case SDLK_KP6:
    b.R_DPAD = 0;
    break;
      case SDLK_KP4:
    b.L_DPAD = 0;
    break;
      case SDLK_KP2:
    b.D_DPAD = 0;
    break;
      case SDLK_KP8:
    b.U_DPAD = 0;
    break;
      case SDLK_RETURN:
    b.START_BUTTON = 0;
    break;
      case SDLK_SPACE:
    b.Z_TRIG = 0;
    break;
      case SDLK_x:
    b.B_BUTTON = 0;
    break;
      case SDLK_w:
    b.A_BUTTON = 0;
    break;
      case SDLK_PAGEDOWN:
    b.R_CBUTTON = 0;
    break;
      case SDLK_DELETE:
    b.L_CBUTTON = 0;
    break;
      case SDLK_END:
    b.D_CBUTTON = 0;
    break;
      case SDLK_HOME:
    b.U_CBUTTON = 0;
    break;
      case SDLK_s:
    b.R_TRIG = 0;
    break;
      case SDLK_q:
    b.L_TRIG = 0;
    break;
      case SDLK_LSHIFT:
    shift_state = 0;
    update_analogic_state();
    break;
      case SDLK_LCTRL:
    ctrl_state = 0;
    update_analogic_state();
    break;
      case SDLK_UP:
    up_state = 0;
    update_analogic_state();
    break;
      case SDLK_DOWN:
    down_state = 0;
    update_analogic_state();
    break;
      case SDLK_RIGHT:
    right_state = 0;
    update_analogic_state();
    break;
      case SDLK_LEFT:
    left_state = 0;
    update_analogic_state();
    break;
     }
}

