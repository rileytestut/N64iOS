/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - input.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Scott Gorman (okaygo)                              *
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

#include <limits.h>
#include <string.h>
#include "input.h"
#include "Input_1.1.h"

#include "SDL.h"

char pluginName[] = "No Input";
char configdir[PATH_MAX] = {0};

int __disableaccel;
int __acceldigital;
unsigned long btUsed;
unsigned long gp2x_pad_status;
long gp2x_pad_x_axis;
long gp2x_pad_y_axis;
long gp2x_pad_z_axis;

typedef struct
{
    int button_a, button_b;         // up/down or left/right; -1 if not assigned
    SDLKey key_a, key_b;            // up/down or left/right; SDLK_UNKNOWN if not assigned
    int axis_a, axis_b;             // axis index; -1 if not assigned
    int axis_dir_a, axis_dir_b;     // direction (1 = X+, 0, -1 = X-)
    int hat, hat_pos_a, hat_pos_b;  // hat + hat position up/down and left/right; -1 if not assigned
} SAxisMap;

SAxisMap      axis[2];      //  2 axis
SDL_Joystick *joystick;     // SDL joystick device


enum  { GP2X_UP=0x1,       GP2X_LEFT=0x4,       GP2X_DOWN=0x10,  GP2X_RIGHT=0x40,
	      GP2X_START=1<<8,   GP2X_SELECT=1<<9,    GP2X_L=1<<10,    GP2X_R=1<<11,
	      GP2X_A=1<<12,      GP2X_B=1<<13,        GP2X_X=1<<14,    GP2X_Y=1<<15,
        GP2X_VOL_UP=1<<23, GP2X_VOL_DOWN=1<<22, GP2X_PUSH=1<<27 };
        
static unsigned short button_bits[] = {
    0x0001,  // R_DPAD
    0x0002,  // L_DPAD
    0x0004,  // D_DPAD
    0x0008,  // U_DPAD
    0x0010,  // START_BUTTON
    0x0020,  // Z_TRIG
    0x0040,  // B_BUTTON
    0x0080,  // A_BUTTON
    0x0100,  // R_CBUTTON
    0x0200,  // L_CBUTTON
    0x0400,  // D_CBUTTON
    0x0800,  // U_CBUTTON
    0x1000,  // R_TRIG
    0x2000,  // L_TRIG
    0x4000,  // Mempak switch
    0x8000   // Rumblepak switch
};

#ifndef __LINUX__

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}
#else
void _init( void )
{
}
#endif // !__LINUX__

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
    PluginInfo->Version = 0x0101;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
    strcpy( PluginInfo->Name, pluginName );
    PluginInfo->Reserved1 = FALSE;
    PluginInfo->Reserved2 = FALSE;
}

EXPORT void CALL InitiateControllers (CONTROL_INFO ControlInfo)
{
    ControlInfo.Controls[0].Present = TRUE;
}

EXPORT void CALL GetKeys(int Control, BUTTONS * Keys )
{
  unsigned long keys = 0;
  
  Keys->Value = 0;
  
  if(__disableaccel)
  {
    Keys->X_AXIS = 0;
	Keys->Y_AXIS = 0;
  }
  else
  {
	  if(btUsed)
	  {
	    int temp_val;
    
	    temp_val = gp2x_pad_x_axis;
	    temp_val = (int)(((float)temp_val / 1.6) - 80.0);
	    temp_val *= 7;
	    if(temp_val > 80)
	      temp_val = 80;
	    if(temp_val < -80)
	      temp_val = -80;      
	    Keys->X_AXIS = temp_val;

	    temp_val = gp2x_pad_y_axis;
	    temp_val = (int)(((float)temp_val / 1.6) - 80.0);
	    temp_val *= 6;
	    if(temp_val > 80)
	      temp_val = 80;
	    if(temp_val < -80)
	      temp_val = -80;      
	    Keys->Y_AXIS = -temp_val;
	  }
	  else
	  {
	    int temp_val;

	    // read joystick state
	    SDL_JoystickUpdate();

	    temp_val = SDL_JoystickGetAxis( joystick, 1 ) / 409;
	    temp_val *= 10;
	    if(temp_val > 80)
	      temp_val = 80;
	    if(temp_val < -80)
	      temp_val = -80;      
	    Keys->X_AXIS = temp_val;

	    temp_val = SDL_JoystickGetAxis( joystick, 0 ) / 409;
	    temp_val *= 7;
	    if(temp_val > 80)
	      temp_val = 80;
	    if(temp_val < -80)
	      temp_val = -80;      
	    Keys->Y_AXIS = temp_val;
	  }
  }
    
  if(gp2x_pad_status & GP2X_RIGHT)
  {
    keys |= 0x0001;
    keys |= 0x0100;
  }
  if(gp2x_pad_status & GP2X_LEFT)
  {
    keys |= 0x0002;
    keys |= 0x0200;
  }
  if(gp2x_pad_status & GP2X_DOWN)
  {
    keys |= 0x0004;
    keys |= 0x0400;
  }
  if(gp2x_pad_status & GP2X_UP)
  {
    keys |= 0x0008;
    keys |= 0x0800;
  }

  if(gp2x_pad_status & GP2X_START)
  {
    keys |= 0x0010;
  }
  if(gp2x_pad_status & GP2X_SELECT)
  {
    keys |= 0x0020;
  }

  if(gp2x_pad_status & GP2X_L)
  {
    keys |= 0x2000;
  }
  if(gp2x_pad_status & GP2X_R)
  {
    keys |= 0x1000;
  }

  if(gp2x_pad_status & GP2X_X)
  {
    keys |= 0x0040;
  }
  if(gp2x_pad_status & GP2X_B)
  {
    keys |= 0x0080;
  }
  
  Keys->Value |= keys;
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the
            emulation thread)
  input:    none
  output:   none
*******************************************************************/
void
RomOpen( void )
{
  if(btUsed)
  {
    return;
  }
  // init SDL joystick subsystem
  if( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
      if( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == -1 )
      {
          fprintf( stderr, "[Controller]: Couldn't init SDL joystick subsystem: %s\n", SDL_GetError() );
          return;
      }

  // open joysticks
  joystick = SDL_JoystickOpen( 0 );
  if( joystick == NULL )
      fprintf( stderr, "[Controller]: Couldn't open joystick for controller: %s\n", SDL_GetError() );
      
  fprintf(stderr, "[Controller]: Init'd\n");
  fflush(stderr);  
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/
void
RomClosed( void )
{
  if(btUsed)
  {
    return;
  }

  if( joystick )
  {
      SDL_JoystickClose( joystick );
      joystick = NULL;
  }

  // quit SDL joystick subsystem
  SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}
