#ifndef GLN64_H
#define GLN64_H

#ifndef __LINUX__
#include <windows.h>
//#include <commctrl.h>
#else
# include "../main/winlnxdefs.h"
#endif

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif

//#define DEBUG
//#define RSPTHREAD

#ifndef __LINUX__
extern HWND         hWnd;
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

