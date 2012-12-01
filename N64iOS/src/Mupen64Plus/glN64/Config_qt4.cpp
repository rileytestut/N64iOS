/*
* Copyright (C) 2008 Louai Al-Khanji
*
* This program is free software; you can redistribute it and/
* or modify it under the terms of the GNU General Public Li-
* cence as published by the Free Software Foundation; either
* version 2 of the Licence, or any later version.
*
* This program is distributed in the hope that it will be use-
* ful, but WITHOUT ANY WARRANTY; without even the implied war-
* ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public Licence for more details.
*
* You should have received a copy of the GNU General Public
* Licence along with this program; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
* USA.
*
*/

#include <dlfcn.h>
#include <errno.h>

#include "Config.h"
#include "OpenGL.h"
#include "glN64.h"

#include "ui_gln64config.h"

// following two functions lifted verbatim from gtk port

static const char *pluginDir = 0;

static inline const char *GetPluginDir()
{
    static char path[PATH_MAX];
#ifdef __WIN32__
    strncpy(path, ".", PATH_MAX);
#else
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
#endif // __USE_GNU
    }
#endif // __WIN32__
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

void Config_DoConfig(HWND hParent)
{
    Config_LoadConfig();

    QDialog dialog(QWidget::find(hParent));
    Ui_glN64ConfigDialog ui;
    ui.setupUi(&dialog);

    QString res = QString("%1x%2")
                    .arg(OGL.fullscreenWidth)
                    .arg(OGL.fullscreenHeight);
    ui.resolutionCombo->setEditText(res);
    ui.forceBilinearFilteringCheck->setChecked(OGL.forceBilinear);
    ui.twoExSaiTextureScalingCheck->setChecked(OGL.enable2xSaI);
    ui.anisotropicFilteringCheck->setChecked(OGL.enableAnisotropicFiltering);
    ui.drawFogCheck->setChecked(OGL.fog);
    ui.hwFramebufferTexturesCheck->setChecked(OGL.frameBufferTextures);
    ui.ditheredAlphaTestingCheck->setChecked(OGL.usePolygonStipple);
    ui.bitDepthCombo->setCurrentIndex(OGL.textureBitDepth);
    ui.cacheSizeSpin->setValue(cache.maxBytes / 1024 / 1024);

    // eek copy paste
    if (dialog.exec() == QDialog::Accepted) {
        FILE *f;

        QStringList res = ui.resolutionCombo->currentText().split("x");
        int xres = 640, yres = 480;
        if (res.count() == 2) {
            bool x_ok, y_ok;
            int newxres = res[0].toInt(&x_ok);
            int newyres = res[1].toInt(&y_ok);
            if (x_ok && y_ok) {
                xres = newxres;
                yres = newyres;
            }
        }
        OGL.fullscreenWidth = OGL.windowedWidth = xres;
        OGL.fullscreenHeight = OGL.windowedHeight = yres;

        OGL.forceBilinear = ui.forceBilinearFilteringCheck->isChecked();
        OGL.enable2xSaI = ui.twoExSaiTextureScalingCheck->isChecked();
        OGL.enableAnisotropicFiltering = ui.anisotropicFilteringCheck->isChecked();
        OGL.fog = ui.drawFogCheck->isChecked();
        OGL.frameBufferTextures = ui.hwFramebufferTexturesCheck->isChecked();
        OGL.usePolygonStipple = ui.ditheredAlphaTestingCheck->isChecked();
        OGL.textureBitDepth = ui.bitDepthCombo->currentIndex();
        cache.maxBytes = ui.cacheSizeSpin->value() * 1024 * 1024;

        // write configuration
        if (pluginDir == 0)
            pluginDir = GetPluginDir();

        char filename[PATH_MAX];
        snprintf( filename, PATH_MAX, "%s/glN64.conf", pluginDir );
        f = fopen( filename, "w" );
        if (!f)
        {
            fprintf( stderr, "[glN64]: (EE) Couldn't save config file '%s': %s\n", filename, strerror( errno ) );
            return;
        }

        fprintf( f, "width=%d\n",        OGL.windowedWidth );
        fprintf( f, "height=%d\n",       OGL.windowedHeight );
        fprintf( f, "force bilinear=%d\n",        OGL.forceBilinear );
        fprintf( f, "enable anisotropic=%d\n", OGL.enableAnisotropicFiltering );
        fprintf( f, "enable 2xSAI=%d\n",          OGL.enable2xSaI );
        fprintf( f, "enable fog=%d\n",            OGL.fog );
        fprintf( f, "enable HardwareFB=%d\n",     OGL.frameBufferTextures );
        fprintf( f, "enable dithered alpha=%d\n", OGL.usePolygonStipple );
        fprintf( f, "texture depth=%d\n",         OGL.textureBitDepth );
        fprintf( f, "cache size=%d\n",            cache.maxBytes / 1048576 );

        fclose( f );
    }
}

