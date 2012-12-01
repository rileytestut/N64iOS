#ifndef GLN64_H
#define GLN64_H

#include "winlnxdefs.h"
#include "stdio.h"


#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

//#define DEBUG
//#define RSPTHREAD

extern HWND         hWnd;
#ifndef __LINUX__
//extern HWND           hFullscreen;
extern HWND         hStatusBar;
extern HWND         hToolBar;
extern HINSTANCE    hInstance;
#endif // !__LINUX__

extern char         pluginName[];

extern void (*CheckInterrupts)( void );
extern char *screenDirectory;
extern char configdir[PATH_MAX];
extern void (*renderCallback)();


#endif

