/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - Config_nogui.cpp                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

#include "Config.h"
#include "glN64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

#include "../main/winlnxdefs.h"

#include "OpenGL.h"
#include "Config.h"

static const char *pluginDir = 0;

static inline const char *GetPluginDir()
{
        static char path[PATH_MAX];

    if(strlen(configdir) > 0)
    {
        strncpy(path, configdir, PATH_MAX);
        // remove trailing '/'
        if(path[strlen(path)-1] == '/')
            path[strlen(path)-1] = '\0';
    }
    else
    {
#ifdef __USE_GNU
                Dl_info info;
                void *addr = (void*)GetPluginDir;
                if(dladdr(addr, &info) != 0)
                {
                   strncpy(path, info.dli_fname, PATH_MAX);
                   *(strrchr(path, '/')) = '\0';
                }
                else
                {
                   fprintf(stderr, "(WW) Couldn't get path of .so, trying to get emulator's path\n");
#endif // __USE_GNU
                   if(readlink("/proc/self/exe", path, PATH_MAX) == -1)
                   {
                           fprintf(stderr, "(WW) readlink() /proc/self/exe failed: %s\n", strerror(errno));
                      path[0] = '.';
                      path[1] = '\0';
                   }
                   *(strrchr(path, '/')) = '\0';
                   strncat(path, "/plugins", PATH_MAX);
#ifdef __USE_GNU
        }
#endif
    }
        return path;
}

void Config_LoadConfig()
{
    static int loaded = 0;
    FILE *f;
    char line[2000];

    if (loaded)
        return;

    loaded = 1;

    if (pluginDir == 0)
        pluginDir = GetPluginDir();

    // default configuration
    OGL.fullscreenWidth = 640;
    OGL.fullscreenHeight = 480;
//  OGL.fullscreenBits = 0;
    OGL.windowedWidth = 640;
    OGL.windowedHeight = 480;
//  OGL.windowedBits = 0;
    OGL.forceBilinear = 0;
    OGL.enable2xSaI = 0;
    OGL.enableAnisotropicFiltering = 0;
    OGL.fog = 1;
    OGL.textureBitDepth = 1; // normal (16 & 32 bits)
    OGL.frameBufferTextures = 0;
    OGL.usePolygonStipple = 0;
    cache.maxBytes = 32 * 1048576;

    // read configuration
    char filename[PATH_MAX];
    snprintf( filename, PATH_MAX, "%s/glN64.conf", pluginDir );
    f = fopen( filename, "r" );
    if (!f)
    {
        fprintf( stderr, "[glN64]: (WW) Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
        return;
    }

    while (!feof( f ))
    {
        char *val;
        fgets( line, 2000, f );

        val = strchr( line, '=' );
        if (!val)
            continue;
        *val++ = '\0';

/*      if (!strcasecmp( line, "fullscreen width" ))
        {
            OGL.fullscreenWidth = atoi( val );
        }
        else if (!strcasecmp( line, "fullscreen height" ))
        {
            OGL.fullscreenHeight = atoi( val );
        }
        else if (!strcasecmp( line, "fullscreen depth" ))
        {
            OGL.fullscreenBits = atoi( val );
        }
        else if (!strcasecmp( line, "windowed width" ))
        {
            OGL.windowedWidth = atoi( val );
        }
        else if (!strcasecmp( line, "windowed height" ))
        {
            OGL.windowedHeight = atoi( val );
        }
        else if (!strcasecmp( line, "windowed depth" ))
        {
            OGL.windowedBits = atoi( val );
        }*/
        if (!strcasecmp( line, "width" ))
        {
            int w = atoi( val );
            OGL.fullscreenWidth = OGL.windowedWidth = (w == 0) ? (640) : (w);
        }
        else if (!strcasecmp( line, "height" ))
        {
            int h = atoi( val );
            OGL.fullscreenHeight = OGL.windowedHeight = (h == 0) ? (480) : (h);
        }
        else if (!strcasecmp( line, "force bilinear" ))
        {
            OGL.forceBilinear = atoi( val );
        }
        else if (!strcasecmp( line, "enable 2xSAI" ))
        {
            OGL.enable2xSaI = atoi( val );
        }
        else if (!strcasecmp( line, "enable anisotropic"))
        {
            OGL.enableAnisotropicFiltering = atoi( val );
        }
        else if (!strcasecmp( line, "enable fog" ))
        {
            OGL.fog = atoi( val );
        }
        else if (!strcasecmp( line, "cache size" ))
        {
            cache.maxBytes = atoi( val ) * 1048576;
        }
        else if (!strcasecmp( line, "enable HardwareFB" ))
        {
            OGL.frameBufferTextures = atoi( val );
        }
        else if (!strcasecmp( line, "enable dithered alpha" ))
        {
            OGL.usePolygonStipple = atoi( val );
        }
        else if (!strcasecmp( line, "texture depth" ))
        {
            OGL.textureBitDepth = atoi( val );
        }
        else
        {
            printf( "Unknown config option: %s\n", line );
        }
    }

    fclose( f );
}

void Config_DoConfig(HWND /*hParent*/)
{
printf("GlN64 compiled with GUI=NONE. Please edit the config file\nin SHAREDIR/glN64.conf manually or recompile plugin with a GUI.\n");
}

