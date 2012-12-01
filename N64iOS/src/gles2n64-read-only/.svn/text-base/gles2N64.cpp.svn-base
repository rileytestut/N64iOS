
#include <string.h>
#include "winlnxdefs.h"

#include "gles2N64.h"
#include "Debug.h"
#include "Zilmar GFX 1.3.h"
#include "OpenGL.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "VI.h"
#include "Config.h"
#include "Textures.h"
#include "ShaderCombiner.h"

HWND        hWnd;
#ifndef __LINUX__
HWND        hStatusBar;
//HWND      hFullscreen;
HWND        hToolBar;
HINSTANCE   hInstance;
#endif // !__LINUX__

char        pluginName[] = "gles2N64 v0.0.2";
char        *screenDirectory;
u32         last_good_ucode = (u32) -1;
void        (*CheckInterrupts)( void );
char        configdir[PATH_MAX] = {0};
void        (*renderCallback)() = NULL;

void _init( void )
{
    Config_LoadConfig();
#ifdef RSPTHREAD
    RSP.thread = NULL;
#endif
}

EXPORT void CALL CaptureScreen ( char * Directory )
{
    screenDirectory = Directory;
#ifdef RSPTHREAD
    if (RSP.thread)
    {
        RSP.threadIdle = 0;
        RSP.threadEvents.push(RSPMSG_CAPTURESCREEN);
        while(!RSP.threadIdle){SDL_Delay(1);};
    }
#else
    OGL_SaveScreenshot();
#endif
}

EXPORT void CALL ChangeWindow (void)
{
}

EXPORT void CALL CloseDLL (void)
{
}

EXPORT void CALL DllAbout ( HWND hParent )
{
    puts( "gles2N64 by Adventus / Orkin\nThanks to Clements, Rice, Gonetz, Malcolm, Dave2001, cryhlove, icepir8, zilmar, Azimer, and StrmnNrmn\nported by blight" );
}

EXPORT void CALL DllConfig ( HWND hParent )
{
    Config_DoConfig(hParent);
}

EXPORT void CALL DllTest ( HWND hParent )
{
}

EXPORT void CALL DrawScreen (void)
{
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{

    PluginInfo->Version = 0x101;
    PluginInfo->Type = PLUGIN_TYPE_GFX;
    strcpy( PluginInfo->Name, pluginName);
    PluginInfo->NormalMemory = FALSE;
    PluginInfo->MemoryBswaped = TRUE;
}

EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{
    hWnd = Gfx_Info.hWnd;

    Config_LoadConfig();
# ifdef RSPTHREAD
    RSP.thread = NULL;
# endif

    DMEM = Gfx_Info.DMEM;
    IMEM = Gfx_Info.IMEM;
    RDRAM = Gfx_Info.RDRAM;

    REG.MI_INTR = (u32*) Gfx_Info.MI_INTR_REG;
    REG.DPC_START = (u32*) Gfx_Info.DPC_START_REG;
    REG.DPC_END = (u32*) Gfx_Info.DPC_END_REG;
    REG.DPC_CURRENT = (u32*) Gfx_Info.DPC_CURRENT_REG;
    REG.DPC_STATUS = (u32*) Gfx_Info.DPC_STATUS_REG;
    REG.DPC_CLOCK = (u32*) Gfx_Info.DPC_CLOCK_REG;
    REG.DPC_BUFBUSY = (u32*) Gfx_Info.DPC_BUFBUSY_REG;
    REG.DPC_PIPEBUSY = (u32*) Gfx_Info.DPC_PIPEBUSY_REG;
    REG.DPC_TMEM = (u32*) Gfx_Info.DPC_TMEM_REG;

    REG.VI_STATUS = (u32*) Gfx_Info.VI_STATUS_REG;
    REG.VI_ORIGIN = (u32*) Gfx_Info.VI_ORIGIN_REG;
    REG.VI_WIDTH = (u32*) Gfx_Info.VI_WIDTH_REG;
    REG.VI_INTR = (u32*) Gfx_Info.VI_INTR_REG;
    REG.VI_V_CURRENT_LINE = (u32*) Gfx_Info.VI_V_CURRENT_LINE_REG;
    REG.VI_TIMING = (u32*) Gfx_Info.VI_TIMING_REG;
    REG.VI_V_SYNC = (u32*) Gfx_Info.VI_V_SYNC_REG;
    REG.VI_H_SYNC = (u32*) Gfx_Info.VI_H_SYNC_REG;
    REG.VI_LEAP = (u32*) Gfx_Info.VI_LEAP_REG;
    REG.VI_H_START = (u32*) Gfx_Info.VI_H_START_REG;
    REG.VI_V_START = (u32*) Gfx_Info.VI_V_START_REG;
    REG.VI_V_BURST = (u32*) Gfx_Info.VI_V_BURST_REG;
    REG.VI_X_SCALE = (u32*) Gfx_Info.VI_X_SCALE_REG;
    REG.VI_Y_SCALE = (u32*) Gfx_Info.VI_Y_SCALE_REG;

    CheckInterrupts = Gfx_Info.CheckInterrupts;

    return TRUE;
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
}


EXPORT void CALL ProcessDList(void)
{
    OGL.frame++;
    if (OGL.frame % OGL.frameSkip == 0 || OGL.frameSkip == 0)
    {
#ifdef RSPTHREAD
        if (RSP.thread)
        {
            RSP.threadIdle = 0;
            RSP.threadEvents.push(RSPMSG_PROCESSDLIST);
            //while(!RSP.threadIdle){SDL_Delay(1);};
        }
#else
        RSP_ProcessDList();
#endif
    }
    else
    {
        RSP.busy = FALSE;
        RSP.DList++;
    }
}

EXPORT void CALL ProcessRDPList(void)
{

}

EXPORT void CALL RomClosed (void)
{
#ifdef RSPTHREAD
    int i;
    if (RSP.thread)
    {
//      if (OGL.fullscreen)
//          ChangeWindow();

        if (RSP.busy)
        {
            RSP.halt = TRUE;
            RSP.threadIdle = 0;
            while(!RSP.threadIdle){SDL_Delay(1);};
        }

        RSP.threadIdle = 0;
        RSP.threadEvents.push(RSPMSG_CLOSE);
        while(!RSP.threadIdle){SDL_Delay(1);};
        SDL_KillThread(RSP.thread);
    }

    RSP.thread = NULL;
#else
    OGL_Stop();
#endif

#ifdef DEBUG
    CloseDebugDlg();
#endif
}

EXPORT void CALL RomOpen (void)
{
#ifdef RSPTHREAD
    RSP.threadIdle = 1;
    while(!RSP.threadEvents.empty()){RSP.threadEvents.pop();}
    RSP.thread = SDL_CreateThread(RSP_ThreadProc, NULL);
#else
    RSP_Init();
#endif

    OGL_ResizeWindow();

#ifdef DEBUG
    OpenDebugDlg();
#endif
}

EXPORT void CALL ShowCFB (void)
{
}

EXPORT void CALL UpdateScreen (void)
{
#ifdef RSPTHREAD
    if (RSP.thread)
    {
        RSP.threadIdle = 0;
        RSP.threadEvents.push(RSPMSG_UPDATESCREEN);
//        while(!RSP.threadIdle){SDL_Delay(1);};
    }
#else
    VI_UpdateScreen();
#endif
}

EXPORT void CALL ViStatusChanged (void)
{
}

EXPORT void CALL ViWidthChanged (void)
{
}


EXPORT void CALL ReadScreen (void **dest, int *width, int *height)
{
    OGL_ReadScreen( dest[0], width, height );
}

EXPORT void CALL SetConfigDir (char *configDir)
{
    strncpy(configdir, configDir, PATH_MAX);
}

EXPORT void CALL SetRenderingCallback(void (*callback)())
{
    renderCallback = callback;
}

