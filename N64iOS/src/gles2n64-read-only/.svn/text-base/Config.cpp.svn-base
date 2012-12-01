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
#include <unistd.h>

#include "Config.h"
#include "gles2N64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

#include "winlnxdefs.h"
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
        strcat(path, ".");
    }

    return path;
}

void Config_WriteConfig(const char *filename)
{
    FILE* f = fopen(filename, "w");
    if (!f)
    {
        printf("Could Not Open %s for writing\n", filename);
    }
    fprintf(f, "#gles2n64 Graphics Plugin for N64\n");
    fprintf(f, "#by Lachlan Tychsen-Smith AKA Adventus\n\n");

    fprintf(f, "#Screen Width in Pixels\n");
    fprintf(f, "screen width=%i\n\n", OGL.screenWidth);

    fprintf(f, "#Screen Height in Pixels\n");
    fprintf(f, "screen height=%i\n\n", OGL.screenHeight);

    fprintf(f, "#Window Width in Pixels\n");
    fprintf(f, "width=%i\n\n", OGL.width);

    fprintf(f, "#Window Height in Pixels\n");
    fprintf(f, "height=%i\n\n", OGL.height);

    fprintf(f, "#Horizontal Position\n");
    fprintf(f, "xpos=%i\n\n", OGL.xpos);

    fprintf(f, "#Vertical Position\n");
    fprintf(f, "ypos=%i\n\n", OGL.ypos);

    fprintf(f, "#Automatically Centre Screen Position (overrides xpos/ypos)\n");
    fprintf(f, "centre=%i\n\n", OGL.centre);

    fprintf(f, "#Enable Frameskip (1=No Frameskip, 2=One Frameskipped, etc)\n");
    fprintf(f, "frame skip=%i\n\n", OGL.frameSkip);

    fprintf(f, "#Vertical Sync Divider (0=No VSYNC, 1=60Hz, 2=30Hz, etc)\n");
    fprintf(f, "vertical sync=%i\n\n", OGL.vSync);

    fprintf(f, "#Texture Bit Depth (0=force 16bit, 1=force 32bit, 2=either 16/32bit) \n");
    fprintf(f, "texture depth=%i\n\n", OGL.textureBitDepth);

    fprintf(f, "#Texture Mipmaps \n");
    fprintf(f, "texture mipmap=%i\n\n", OGL.textureMipmap);

    fprintf(f, "#Enable Fog\n");
    fprintf(f, "enable fog=%i\n\n", OGL.enableFog);

    fprintf(f, "#Enable Lighting\n");
    fprintf(f, "enable lighting=%i\n\n", OGL.enableLighting);

    fprintf(f, "#Enable Alpha Test\n");
    fprintf(f, "enable alpha test=%i\n\n", OGL.enableAlphaTest);

    fprintf(f, "#Enable Clipping\n");
    fprintf(f, "enable clipping=%i\n\n", OGL.enableClipping);

    fprintf(f, "#Enable 2xSAI Texture Filter \n");
    fprintf(f, "enable 2xSAI=%i\n\n", OGL.enable2xSaI);

    fprintf(f, "#Enable Anisotropic Filtering \n");
    fprintf(f, "enable anisotropic=%i\n\n", OGL.enableAnisotropicFiltering);

    fprintf(f, "#Max Anisotropy \n");
    fprintf(f, "max anisotropy=%i\n\n", OGL.maxAnisotropy);

    fprintf(f, "#Enable Accurate RDP Clamping (do not enable by default) \n");
    fprintf(f, "accurate rdp=%i\n\n", OGL.accurateRDP);

    fclose(f);
}

void Config_LoadConfig()
{
    static int loaded = 0;
    FILE *f;
    char line[4096];

    if (loaded)
        return;

    loaded = 1;

    if (pluginDir == 0)
        pluginDir = GetPluginDir();

    // default configuration
    OGL.xpos = 0;
    OGL.ypos = 0;
    OGL.width = 800;
    OGL.height = 480;
    OGL.screenWidth = 800;
    OGL.screenHeight = 480;
    OGL.centre = 1;
    OGL.frameSkip = 1;
    OGL.vSync = 0;
    OGL.enableLighting = 1;
    OGL.enableAlphaTest = 1;
    OGL.enableClipping = 0;
    OGL.enableFog = 0;
    OGL.accurateRDP = 0;
    OGL.forceBilinear = 0;
    OGL.enable2xSaI = 0;
    OGL.enableAnisotropicFiltering = 0;
    OGL.maxAnisotropy = 2;
    OGL.textureBitDepth = 1; // normal (16 & 32 bits)
    OGL.textureMipmap = 1;
    OGL.logFrameRate = 1;
    cache.maxBytes = 32 * 1048576;

    // read configuration
    char filename[PATH_MAX];
    snprintf( filename, PATH_MAX, "%s/gles2n64.conf", pluginDir );
    f = fopen( filename, "r" );
    if (!f)
    {
        fprintf( stdout, "[gles2N64]: (WW) Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
        fprintf( stdout, "[gles2N64]: Attempting to write new Config \n");
        Config_WriteConfig(filename);
        return;
    }
    else
    {
        printf("[gles2n64]: Loading Config from %s \n", filename);
    }

    while (!feof( f ))
    {
        char *val;
        fgets( line, 4096, f );

        if (line[0] == '#' || line[0] == '\n')
            continue;

        val = strchr( line, '=' );
        if (!val) continue;

        *val++ = '\0';

        if (!strcasecmp( line, "width" ))
        {
            OGL.width = atoi( val );
        }
        else if (!strcasecmp( line, "height" ))
        {
            OGL.height = atoi( val );
        }
        else if (!strcasecmp( line, "screen width" ))
        {
            OGL.screenWidth = atoi( val );
        }
        else if (!strcasecmp( line, "screen height" ))
        {
            OGL.screenHeight = atoi( val );
        }
        else if (!strcasecmp( line, "centre" ))
        {
            OGL.centre = atoi( val );
        }
        else if (!strcasecmp( line, "xpos" ))
        {
            OGL.xpos = atoi(val);
        }
        else if (!strcasecmp( line, "ypos" ))
        {
            OGL.ypos = atoi(val);
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
        else if (!strcasecmp( line, "max anisotropy"))
        {
            OGL.maxAnisotropy = atoi( val );
        }
        else if (!strcasecmp( line, "enable fog" ))
        {
            OGL.enableFog = atoi( val );
        }
        else if (!strcasecmp( line, "enable clipping" ))
        {
            OGL.enableClipping = atoi( val );
        }
        else if (!strcasecmp( line, "enable lighting" ))
        {
            OGL.enableLighting = atoi( val );
        }
        else if (!strcasecmp( line, "enable alpha test" ))
        {
            OGL.enableAlphaTest = atoi( val );
        }
        else if (!strcasecmp( line, "cache size" ))
        {
            cache.maxBytes = atoi( val ) * 1048576;
        }
        else if (!strcasecmp( line, "texture depth" ))
        {
            OGL.textureBitDepth = atoi( val );
        }
        else if (!strcasecmp( line, "texture mipmap" ))
        {
            OGL.textureMipmap = atoi( val );
        }
        else if (!strcasecmp( line, "frame skip" ))
        {
            OGL.frameSkip = atoi( val );
        }
        else if (!strcasecmp( line, "vertical sync" ))
        {
            OGL.vSync = atoi( val );
        }
        else if (!strcasecmp( line, "log fps" ))
        {
            OGL.logFrameRate = atoi( val );
        }
        else if (!strcasecmp( line, "accurate RDP" ))
        {
            OGL.accurateRDP = atoi(val);
        }
        else
        {
            printf( "[gles2n64]: Unsupported config option: %s\n", line );
        }
    }

    if (OGL.centre)
    {
        OGL.xpos = (OGL.screenWidth - OGL.width) / 2;
        OGL.ypos = (OGL.screenHeight - OGL.height) / 2;
    }

    fclose( f );
}

void Config_DoConfig(HWND)
{
    printf("Please edit the config file gles2n64.conf manually\n");
}

