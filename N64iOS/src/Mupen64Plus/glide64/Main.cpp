/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public
*   Licence along with this program; if not, write to the Free
*   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
*   Boston, MA  02110-1301, USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators (tested mostly with Project64)
// Project started on December 29th, 2001
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
// Official Glide64 development channel: #Glide64 on EFnet
//
// Original author: Dave2001 (Dave2999@hotmail.com)
// Other authors: Gonetz, Gugaman
//
//****************************************************************

#include "Gfx1.3.h"
#include "Util.h"
#include "3dmath.h"
#include "Debugger.h"

#include "Combine.h"

#include "Ini.h"

#include "TexCache.h"
#include "CRC.h"
#include "DepthBufferRender.h"

#include <string.h>
#include <stdlib.h>
#include "messagebox.h"
#include <sys/time.h>

#define G64_VERSION "Mupen64Plus"
#define RELTIME "Date: " __DATE__ " Time: " __TIME__

#ifdef EXT_LOGGING
std::ofstream extlog;
#endif

#ifdef LOGGING
std::ofstream loga;
#endif

#ifdef RDP_LOGGING
BOOL log_open = FALSE;
std::ofstream rdp_log;
#endif

#ifdef RDP_ERROR_LOG
BOOL elog_open = FALSE;
std::ofstream rdp_err;
#endif

GFX_INFO gfx;

BOOL to_fullscreen = FALSE;
BOOL fullscreen = FALSE;
BOOL romopen = FALSE;
GrContext_t gfx_context = 0;
BOOL debugging = FALSE;
HINSTANCE hInstance = NULL;
BOOL exception = FALSE;

BOOL evoodoo = 0;
BOOL ev_fullscreen = 0;

int num_tmu;
int max_tex_size;
long sup_mirroring;
BOOL sup_32bit_tex = FALSE;

#ifdef ALTTAB_FIX
HHOOK hhkLowLevelKybd = NULL;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, 
                                      WPARAM wParam, LPARAM lParam);
#endif

#ifdef PERFORMANCE
__int64 perf_cur;
__int64 perf_next;
#endif

#ifdef FPS
LARGE_INTEGER perf_freq;
LARGE_INTEGER fps_last;
LARGE_INTEGER fps_next;
float     fps = 0.0f;
DWORD     fps_count = 0;

DWORD     vi_count = 0;
float     vi = 0.0f;

DWORD     region = 0;

float     ntsc_percent = 0.0f;
float     pal_percent = 0.0f;

#endif

// Resolutions, MUST be in the correct order (SST1VID.H)
DWORD resolutions[0x18][2] = {
  { 320, 200 },
  { 320, 240 },
  { 400, 256 },
  { 512, 384 },
  { 640, 200 },
  { 640, 350 },
  { 640, 400 },
  { 640, 480 },
  { 800, 600 },
  { 960, 720 },
  { 856, 480 },
  { 512, 256 },
  { 1024, 768 },
  { 1280, 1024 },
  { 1600, 1200 },
  { 400, 300 },
  
  // 0x10
  { 1152, 864 },
  { 1280, 960 },
  { 1600, 1024 },
  { 1792, 1344 },
  { 1856, 1392 },
  { 1920, 1440 },
  { 2048, 1536 },
  { 2048, 2048 }
};

// ref rate
// 60=0x0, 70=0x1, 72=0x2, 75=0x3, 80=0x4, 90=0x5, 100=0x6, 85=0x7, 120=0x8, none=0xff

unsigned long BMASK = 0x7FFFFF;
// Reality display processor structure
RDP rdp;

SETTINGS settings = { FALSE, 640, 480, GR_RESOLUTION_640x480, 0 };

HOTKEY_INFO hotkey_info;

GrTexInfo fontTex;
GrTexInfo cursorTex;
DWORD   offset_font = 0;
DWORD   offset_cursor = 0;
DWORD   offset_textures = 0;
DWORD   offset_texbuf1 = 0;

BOOL    capture_screen = 0;
char    capture_path[256];

void (*renderCallback)() = NULL;


void ChangeSize ()
{
  float res_scl_x = (float)settings.res_x / 320.0f;
  float res_scl_y = (float)settings.res_y / 240.0f;
  
  DWORD scale_x = *gfx.VI_X_SCALE_REG & 0xFFF;
  if (!scale_x) return;
  DWORD scale_y = *gfx.VI_Y_SCALE_REG & 0xFFF;
  if (!scale_y) return;
  
  float fscale_x = (float)scale_x / 1024.0f;
  float fscale_y = (float)scale_y / 1024.0f;
  
  DWORD dwHStartReg = *gfx.VI_H_START_REG;
  DWORD dwVStartReg = *gfx.VI_V_START_REG;
  
  DWORD hstart = dwHStartReg >> 16;
  DWORD hend = dwHStartReg & 0xFFFF;
  
  // dunno... but sometimes this happens
  if (hend == hstart) hend = (int)(*gfx.VI_WIDTH_REG / fscale_x);
  
  DWORD vstart = dwVStartReg >> 16;
  DWORD vend = dwVStartReg & 0xFFFF;
  
  sprintf (out_buf, "hstart: %d, hend: %d, vstart: %d, vend: %d\n", hstart, hend, vstart, vend);
  LOG (out_buf);
  
  rdp.vi_width = (hend - hstart) * fscale_x;
  rdp.vi_height = (vend - vstart)/2 * fscale_y;
  
  sprintf (out_buf, "size: %d x %d\n", (int)rdp.vi_width, (int)rdp.vi_height);
  LOG (out_buf);
  
  if (region == 0)
  {
    if (*gfx.VI_WIDTH_REG == 0x500) // 1280x960 is different... needs height * 2
    {
      rdp.scale_x = res_scl_x * (320.0f / rdp.vi_width);
      rdp.scale_y = res_scl_y * (120.0f / rdp.vi_height);
    }
    else
    {
      rdp.scale_x = res_scl_x * (320.0f / rdp.vi_width);
      rdp.scale_y = res_scl_y * (240.0f / rdp.vi_height);
    }
  }
  else
  {
    // odd... but pal games seem to want 230 as height...
    if (*gfx.VI_WIDTH_REG == 0x500) // 1280x960 is different... needs height * 2
    {
      // NOT SURE ABOUT PAL HERE, DON'T HAVE PAL MEGAMAN TO TRY
      rdp.scale_x = res_scl_x * (320.0f / rdp.vi_width);
      // VP changed to 120
      rdp.scale_y = res_scl_y * (120.0f / rdp.vi_height);
      //rdp.scale_y = res_scl_y * (115.0f / rdp.vi_height);
    }
    else
    {
      rdp.scale_x = res_scl_x * (320.0f / rdp.vi_width);
      // VP changed to 240
      rdp.scale_y = res_scl_y * (240.0f / rdp.vi_height);
      //rdp.scale_y = res_scl_y * (230.0f / rdp.vi_height);
    }
  }
  
  rdp.offset_x = settings.offset_x * res_scl_x;
  rdp.offset_y = settings.offset_y * res_scl_y;
  if (settings.scale_x != 0)
    rdp.scale_x *= (settings.scale_x / 100000.0f);
  if (settings.scale_y != 0)
    rdp.scale_y *= (settings.scale_y / 100000.0f);
  
  rdp.scale_1024 = settings.scr_res_x / 1024.0f;
  rdp.scale_768 = settings.scr_res_y / 768.0f;
  
  rdp.scissor_o.ul_x = 0;
  rdp.scissor_o.ul_y = 0;
  rdp.scissor_o.lr_x = (DWORD)rdp.vi_width;
  rdp.scissor_o.lr_y = (DWORD)rdp.vi_height;
  
  rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
}

void ReadSettings ()
{
  //  LOG("ReadSettings\n");
  INI_Open ();
  INI_FindSection ("SETTINGS");
  settings.card_id = (BYTE)INI_ReadInt ("card_id", 0);
  
  settings.depth_bias = -INI_ReadInt ("depth_bias", 0);
  settings.res_data = (DWORD) INI_ReadInt ("resolution", 7);
  if (settings.res_data >= 24) settings.res_data = 12;
  settings.scr_res_x = settings.res_x = resolutions[settings.res_data][0];
  settings.scr_res_y = settings.res_y = resolutions[settings.res_data][1];
  settings.autodetect_ucode = (BOOL)INI_ReadInt ("autodetect_ucode", 1);
  settings.ucode = (DWORD)INI_ReadInt ("ucode", 2);
  
  settings.wireframe = (BOOL)INI_ReadInt ("wireframe", 0);
  settings.wfmode = (int)INI_ReadInt ("wfmode", 1);
  settings.filtering = (BYTE)INI_ReadInt ("filtering", 1);
  settings.fog = (BOOL)INI_ReadInt ("fog", 0);
  settings.buff_clear = (BOOL)INI_ReadInt ("buff_clear", 1);
  settings.vsync = (BOOL)INI_ReadInt ("vsync", 0);
  settings.fast_crc = (BOOL)INI_ReadInt ("fast_crc", 0);
  settings.swapmode = (BYTE)INI_ReadInt ("swapmode", 1);
  settings.lodmode = (BYTE)INI_ReadInt ("lodmode", 0);
  
  settings.logging = (BOOL)INI_ReadInt ("logging", 0);
  settings.log_clear = (BOOL)INI_ReadInt ("log_clear", 0);
  settings.elogging = (BOOL)INI_ReadInt ("elogging", 0);
  settings.filter_cache = (BOOL)INI_ReadInt ("filter_cache", 0);
  settings.cpu_write_hack = (BOOL)INI_ReadInt ("detect_cpu_write", 0);
  settings.unk_as_red = (BOOL)INI_ReadInt ("unk_as_red", 0);
  settings.log_unk = (BOOL)INI_ReadInt ("log_unk", 0);
  settings.unk_clear = (BOOL)INI_ReadInt ("unk_clear", 0);
  
  settings.wrap_big_tex = (BOOL)INI_ReadInt ("wrap_big_tex", 0);
  settings.flame_corona = (BOOL)INI_ReadInt ("flame_corona", 0);
  //  settings.RE2_native_video = (BOOL)INI_ReadInt ("RE2_native_video", 0);
  
  settings.show_fps = (BYTE)INI_ReadInt ("show_fps", 0);
  
  settings.clock = (BOOL)INI_ReadInt ("clock", 0);
  settings.clock_24_hr = (BOOL)INI_ReadInt ("clock_24_hr", 0);
  
  settings.fb_read_always = (BOOL)INI_ReadInt ("fb_read_always", 0);
  settings.fb_read_alpha = (BOOL)INI_ReadInt ("fb_read_alpha", 0);
  settings.fb_smart = (BOOL)INI_ReadInt ("fb_smart", 0);
  settings.fb_motionblur = (BOOL)INI_ReadInt ("motionblur", 0);
  settings.fb_hires = (BOOL)INI_ReadInt ("fb_hires", 0);
  settings.fb_get_info = (BOOL)INI_ReadInt ("fb_get_info", 0);
  settings.fb_depth_clear = (BOOL)INI_ReadInt ("fb_clear", 0);
  settings.fb_depth_render = (BOOL)INI_ReadInt ("fb_render", 0);
  if (settings.fb_depth_render)
    settings.fb_depth_clear = TRUE;
  
  settings.custom_ini = (BOOL)INI_ReadInt ("custom_ini", 0);
  settings.hotkeys = (BOOL)INI_ReadInt ("hotkeys", 0);

  settings.full_res = (BOOL)INI_ReadInt ("full_res", 7);
  settings.tex_filter = (BOOL)INI_ReadInt ("tex_filter", 0);
  settings.noditheredalpha = (BOOL)INI_ReadInt ("noditheredalpha", 0);
  settings.noglsl = (BOOL)INI_ReadInt ("noglsl", 0);
  settings.FBO = (BOOL)INI_ReadInt ("fbo", 0);
  settings.disable_auxbuf = (BOOL)INI_ReadInt ("disable_auxbuf", 0);
  
  INI_Close ();
}

void ReadSpecialSettings (const char name[21])
{
  //  char buf [256];
  //  sprintf(buf, "ReadSpecialSettings. Name: %s\n", name);
  //  LOG(buf);
  settings.zelda = FALSE;    //zeldas hacks
  settings.bomberman64 = FALSE; //bomberman64 hacks
  settings.diddy = FALSE;    //diddy kong racing
  settings.tonic = FALSE;    //tonic trouble
  settings.PPL = FALSE;      //pokemon puzzle league requires many special fixes
  settings.ASB = FALSE;      //All-Star Baseball games
  settings.doraemon2 = FALSE;//Doraemon 2
  settings.invaders = FALSE; //Space Invaders
  settings.BAR = FALSE;      //Beetle Adventure Racing
  settings.ISS64 = FALSE;    //International Superstar Soccer 64
  settings.RE2 = FALSE;      //Resident Evil 2
  settings.nitro = FALSE;    //WCW Nitro
  settings.chopper = FALSE;  //Chopper Attack
  settings.yoshi = FALSE;    // Yoshi Story
  settings.fzero = FALSE;    // F-Zero
  settings.PM = FALSE;       //Paper Mario
  settings.TGR = FALSE;      //Top Gear Rally
  settings.TGR2 = FALSE;     //Top Gear Rally 2
  settings.KI = FALSE;       //Killer Instinct
  settings.lego = FALSE;     //LEGO Racers

  //detect games which require special hacks
  if (strstr(name, (const char *)"ZELDA") || strstr(name, (const char *)"MASK"))
    settings.zelda = TRUE;
  else if (strstr(name, (const char *)"ROADSTERS TROPHY"))
    settings.zelda = TRUE;
  else if (strstr(name, (const char *)"Diddy Kong Racing"))
    settings.diddy = TRUE;
  else if (strstr(name, (const char *)"BOMBERMAN64"))
    settings.bomberman64 = TRUE;
  else if (strstr(name, (const char *)"BAKU-BOMBERMAN"))
    settings.bomberman64 = TRUE;
  else if (strstr(name, (const char *)"Tonic Trouble"))
    settings.tonic = TRUE;
  else if (strstr(name, (const char *)"All") && strstr(name, (const char *)"Star") && strstr(name, (const char *)"Baseball"))
    settings.ASB = TRUE;
  else if (strstr(name, (const char *)"��״��2 ˶�ɼ����"))
    settings.doraemon2 = TRUE;
  else if (strstr(name, (const char *)"SPACE INVADERS"))
    settings.invaders = TRUE;
  else if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV"))
    settings.BAR = TRUE;
  else if (strstr(name, (const char *)"I S S 64") || strstr(name, (const char *)"PERFECT STRIKER"))
    settings.ISS64 = TRUE;
  else if (strstr(name, (const char *)"NITRO64"))
    settings.nitro = TRUE;
  else if (strstr(name, (const char *)"CHOPPER_ATTACK"))
    settings.chopper = TRUE;
  else if (strstr(name, (const char *)"Resident Evil II") || strstr(name, (const char *)"BioHazard II"))
  {
    settings.RE2 = TRUE;
    ZLUT_init();
  }
  else if (strstr(name, (const char *)"YOSHI STORY"))
    settings.yoshi= TRUE;
  else if (strstr(name, (const char *)"F-Zero X") || strstr(name, (const char *)"F-ZERO X"))
    settings.fzero = TRUE;
  else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
    settings.PM = TRUE;
  else if (strstr(name, (const char *)"TOP GEAR RALLY 2"))
    settings.TGR2 = TRUE;
  else if (strstr(name, (const char *)"TOP GEAR RALLY"))
    settings.TGR = TRUE;
  else if (strstr(name, (const char *)"Killer Instinct Gold") || strstr(name, (const char *)"KILLER INSTINCT GOLD"))
    settings.KI = TRUE;
  else if (strstr(name, (const char *)"LEGORacers"))
    settings.lego = TRUE;
  
  INI_Open ();
  if (INI_FindSection (name,FALSE) == FALSE)
  {
    INI_Close ();
    return;
  }
  
  int offset_x = INI_ReadInt ("offset_x", -1, 0);
  int offset_y = INI_ReadInt ("offset_y", -1, 0);
  int scale_x = INI_ReadInt ("scale_x", -1, 0);
  int scale_y = INI_ReadInt ("scale_y", -1, 0);
  int alt_tex_size = INI_ReadInt ("alt_tex_size", -1, 0);
  int use_sts1_only = INI_ReadInt ("use_sts1_only", -1, 0);
  int ppl = INI_ReadInt ("PPL", -1, 0);
  //  int texrect_zbuf = INI_ReadInt ("force_texrect_zbuf", -1, 0);
  int optimize_texrect = INI_ReadInt ("optimize_texrect", -1, 0);
  int optimize_write = INI_ReadInt ("optimize_write", -1, 0);
  int ignore_aux_copy = INI_ReadInt ("ignore_aux_copy", -1, 0);
  int ignore_previous = INI_ReadInt ("ignore_previous", -1, 0);
  int hires_buf_clear = INI_ReadInt ("hires_buf_clear", -1, 0);
  int wrap_big_tex = INI_ReadInt ("wrap_big_tex", -1, 0);
  int tex_fix = INI_ReadInt ("fix_tex_coord", -1, 0);
  int soft_depth_compare = INI_ReadInt ("soft_depth_compare", -1, 0);
  int force_depth_compare = INI_ReadInt ("force_depth_compare", -1, 0);
  int fillcolor_fix = INI_ReadInt ("fillcolor_fix", -1, 0);
  int depth_bias = INI_ReadInt ("depth_bias", -1, 0);
  int increase_texrect_edge = INI_ReadInt ("increase_texrect_edge", -1, 0);
  int decrease_fillrect_edge = INI_ReadInt ("decrease_fillrect_edge", -1, 0);
  int increase_primdepth = INI_ReadInt ("increase_primdepth", -1, 0);
  int stipple_mode = INI_ReadInt ("stipple_mode", -1, 0);
  int stipple_pattern = INI_ReadInt ("stipple_pattern", -1, 0);
  int force_microcheck = INI_ReadInt ("force_microcheck", -1, 0);
  int info_disable = INI_ReadInt ("fb_info_disable", -1, 0);
  int hires_disable = INI_ReadInt ("fb_hires_disable", -1, 0);
  
  if (offset_x != -1) settings.offset_x = offset_x;
  if (offset_y != -1) settings.offset_y = offset_y;
  if (scale_x != -1) settings.scale_x = scale_x;
  if (scale_y != -1) settings.scale_y = scale_y;
  if (alt_tex_size != -1) settings.alt_tex_size = alt_tex_size;
  if (use_sts1_only != -1) settings.use_sts1_only = use_sts1_only;
  if (ppl != -1) settings.PPL = ppl;
  //  if (texrect_zbuf != -1) settings.force_texrect_zbuf = texrect_zbuf;
  if (optimize_texrect != -1) settings.fb_optimize_texrect = optimize_texrect;
  if (optimize_write != -1) settings.fb_optimize_write = optimize_write;
  if (ignore_aux_copy != -1) settings.fb_ignore_aux_copy = ignore_aux_copy;
  if (hires_buf_clear != -1) settings.fb_hires_buf_clear = hires_buf_clear;
  if (wrap_big_tex != -1) settings.wrap_big_tex = wrap_big_tex;
  if (tex_fix != -1) settings.fix_tex_coord = tex_fix;
  if (soft_depth_compare != -1) settings.soft_depth_compare = soft_depth_compare;
  if (force_depth_compare != -1) settings.force_depth_compare = force_depth_compare;
  if (fillcolor_fix != -1) settings.fillcolor_fix = fillcolor_fix;
  if (depth_bias != -1) settings.depth_bias = -depth_bias;
  if (increase_texrect_edge != -1) settings.increase_texrect_edge = increase_texrect_edge;
  if (decrease_fillrect_edge != -1) settings.decrease_fillrect_edge = decrease_fillrect_edge;
  if (increase_primdepth != -1) settings.increase_primdepth = increase_primdepth;
  if (stipple_mode != -1) settings.stipple_mode = stipple_mode;
  if (stipple_pattern != -1) settings.stipple_pattern = (DWORD)stipple_pattern;
  if (force_microcheck != -1) settings.force_microcheck = force_microcheck;
  if (ignore_previous != -1) settings.fb_ignore_previous = ignore_previous;
  if (info_disable == 1) settings.fb_get_info = 0;
  if (hires_disable == 1) settings.fb_hires = 0;
  if (settings.lodmode == 0)
  {
    int lodmode = INI_ReadInt ("lodmode", -1, 0);
    if (lodmode > 0)
      settings.lodmode = lodmode;
  }

  if (settings.custom_ini)
  {
    int filtering = INI_ReadInt ("filtering", -1, 0);
    int fog = INI_ReadInt ("fog", -1, 0);
    int buff_clear = INI_ReadInt ("buff_clear", -1, 0);
    int swapmode = INI_ReadInt ("swapmode", -1, 0);
    int smart_read = INI_ReadInt ("fb_smart", -1, 0);
    int read_alpha = INI_ReadInt ("fb_read_alpha", -1, 0);
    int depth_clear = INI_ReadInt ("fb_clear", -1, 0);
    //FIXME unused int depth_render = INI_ReadInt ("fb_render", -1, 0);
    int resolution = (INT)INI_ReadInt ("resolution", -1, 0);
    int cpu_write_hack = (INT)INI_ReadInt ("detect_cpu_write", -1, 0);
    
    if (filtering != -1) settings.filtering = filtering;
    if (fog != -1) settings.fog = fog;
    if (buff_clear != -1) settings.buff_clear = buff_clear;
    if (swapmode != -1) settings.swapmode= swapmode;
    //    settings.swapmode = 2;
    if (smart_read != -1) settings.fb_smart = smart_read;
    if (read_alpha != -1) settings.fb_read_alpha= read_alpha;
    if (depth_clear != -1) settings.fb_depth_clear = depth_clear;
    if (cpu_write_hack != -1) settings.cpu_write_hack = cpu_write_hack;
    if (resolution != -1)
    {
      settings.res_data = (DWORD) resolution;
      if (settings.res_data >= 0x18) settings.res_data = 12;
      settings.scr_res_x = settings.res_x = resolutions[settings.res_data][0];
      settings.scr_res_y = settings.res_y = resolutions[settings.res_data][1];
    }
  }
  if (settings.fb_depth_render)
    settings.fb_depth_clear = TRUE;
  INI_Close ();
}

//
// WriteRegistry - writes the settings in the registry
//

void WriteSettings ()
{
  INI_Open ();
  INI_FindSection ("SETTINGS");
  INI_WriteInt ("card_id", settings.card_id);
  INI_WriteInt ("resolution", settings.res_data);
  INI_WriteInt ("autodetect_ucode", settings.autodetect_ucode);
  INI_WriteInt ("ucode", settings.ucode);
  
  INI_WriteInt ("wireframe", settings.wireframe);
  INI_WriteInt ("wfmode", settings.wfmode);
  INI_WriteInt ("filtering", settings.filtering);
  INI_WriteInt ("fog", settings.fog);
  INI_WriteInt ("buff_clear", settings.buff_clear);
  INI_WriteInt ("vsync", settings.vsync);
  INI_WriteInt ("fast_crc", settings.fast_crc);
  INI_WriteInt ("swapmode", settings.swapmode);
  INI_WriteInt ("lodmode", settings.lodmode);
  
  INI_WriteInt ("logging", settings.logging);
  INI_WriteInt ("log_clear", settings.log_clear);
  INI_WriteInt ("elogging", settings.elogging);
  INI_WriteInt ("filter_cache", settings.filter_cache);
  INI_WriteInt ("detect_cpu_write", settings.cpu_write_hack);
  INI_WriteInt ("unk_as_red", settings.unk_as_red);
  INI_WriteInt ("log_unk", settings.log_unk);
  INI_WriteInt ("unk_clear", settings.unk_clear);
  
  INI_WriteInt ("wrap_big_tex", settings.wrap_big_tex);
  INI_WriteInt ("flame_corona", settings.flame_corona);
  //  INI_WriteInt ("RE2_native_video", settings.RE2_native_video);
  
  
  INI_WriteInt ("show_fps", settings.show_fps);
  
  INI_WriteInt ("clock", settings.clock);
  INI_WriteInt ("clock_24_hr", settings.clock_24_hr);
  
  INI_WriteInt ("fb_read_always", settings.fb_read_always);
  INI_WriteInt ("fb_read_alpha", settings.fb_read_alpha);
  INI_WriteInt ("fb_smart", settings.fb_smart);
  INI_WriteInt ("motionblur", settings.fb_motionblur);
  INI_WriteInt ("fb_hires", settings.fb_hires);
  INI_WriteInt ("fb_get_info", settings.fb_get_info);
  INI_WriteInt ("fb_clear", settings.fb_depth_clear);
  INI_WriteInt ("fb_render", settings.fb_depth_render);
  
  INI_WriteInt ("custom_ini", settings.custom_ini);
  INI_WriteInt ("hotkeys", settings.hotkeys);

  INI_WriteInt ("full_res", settings.full_res);
  INI_WriteInt ("tex_filter", settings.tex_filter);
  INI_WriteInt ("noditheredalpha", settings.noditheredalpha);
  INI_WriteInt ("noglsl", settings.noglsl);
  INI_WriteInt ("fbo", settings.FBO);
  
  INI_Close ();
}

#include "font.h"
#include "cursor.h"

GRFRAMEBUFFERCOPYEXT grFramebufferCopyExt = NULL;
GRTEXBUFFEREXT   grTextureBufferExt = NULL;
GRTEXBUFFEREXT   grTextureAuxBufferExt = NULL;
GRAUXBUFFEREXT   grAuxBufferExt = NULL;
GRSTIPPLE grStippleModeExt = NULL;
GRSTIPPLE grStipplePatternExt = NULL;
BOOL combineext = FALSE;

BOOL depthbuffersave = FALSE;

// guLoadTextures - used to load the cursor and font textures
void guLoadTextures ()
{
  if (grTextureBufferExt)
  {
    int tbuf_size = 0;
    if (max_tex_size <= 256) 
    {
      grTextureBufferExt(  GR_TMU1, grTexMinAddress(GR_TMU1), GR_LOD_LOG2_256, GR_LOD_LOG2_256, 
                    GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH ); 
      tbuf_size = 8 * grTexCalcMemRequired(GR_LOD_LOG2_256, GR_LOD_LOG2_256, 
                                         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
    }
    else if (settings.scr_res_x <= 1024)
    {
      grTextureBufferExt(  GR_TMU1, grTexMinAddress(GR_TMU1), GR_LOD_LOG2_1024, GR_LOD_LOG2_1024, 
                    GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH ); 
      tbuf_size = grTexCalcMemRequired(GR_LOD_LOG2_1024, GR_LOD_LOG2_1024, 
                                         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
    }
    else
    {
      grTextureBufferExt(  GR_TMU1, grTexMinAddress(GR_TMU1), GR_LOD_LOG2_2048, GR_LOD_LOG2_2048, 
                    GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH ); 
      tbuf_size = grTexCalcMemRequired(GR_LOD_LOG2_2048, GR_LOD_LOG2_2048, 
                                         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
    }
    
    //tbuf_size *= 2;
    printf("tbuf_size %gMb\n", tbuf_size/1024.0f/1024);
    rdp.texbufs[0].tmu = GR_TMU0;
    rdp.texbufs[0].begin = grTexMinAddress(GR_TMU0);
    rdp.texbufs[0].end = rdp.texbufs[0].begin+tbuf_size;
    rdp.texbufs[0].count = 0;
    rdp.texbufs[0].clear_allowed = TRUE;
    if (num_tmu > 1)
    {
      rdp.texbufs[1].tmu = GR_TMU1;
      rdp.texbufs[1].begin = grTexMinAddress(GR_TMU1);
      rdp.texbufs[1].end = rdp.texbufs[1].begin+tbuf_size;
      rdp.texbufs[1].count = 0;
      rdp.texbufs[1].clear_allowed = TRUE;
      offset_texbuf1 = tbuf_size;
    }
    offset_font = tbuf_size;
  }
  else
    offset_font = 0;

   DWORD *data = (DWORD*)font;
   DWORD cur;
  
  // ** Font texture **
  BYTE *tex8 = (BYTE*)malloc(256*64);
  
  fontTex.smallLodLog2 = fontTex.largeLodLog2 = GR_LOD_LOG2_256;
  fontTex.aspectRatioLog2 = GR_ASPECT_LOG2_4x1;
  fontTex.format = GR_TEXFMT_ALPHA_8;
  fontTex.data = tex8;
  
  // Decompression: [1-bit inverse alpha --> 8-bit alpha]
  DWORD i,b;
  for (i=0; i<0x200; i++)
  {
    // cur = ~*(data++), byteswapped
#if !defined(__GNUC__) && !defined(NO_ASM)
    __asm {
      mov eax, dword ptr [data]
        mov ecx, dword ptr [eax]
        add eax, 4
        mov dword ptr [data], eax
        not ecx
        bswap ecx
        mov dword ptr [cur],ecx
    }
#elif !defined(NO_ASM)
     asm volatile ("bswap %[cur]"
           : [cur] "=g"(cur)
           : "[cur]"(~*(data++))
           );
#endif
    
    for (b=0x80000000; b!=0; b>>=1)
    {
      if (cur&b) *tex8 = 0xFF;
      else *tex8 = 0x00;
      tex8 ++;
    }
  }
  
  grTexDownloadMipMap (GR_TMU0,
    grTexMinAddress(GR_TMU0) + offset_font,
    GR_MIPMAPLEVELMASK_BOTH,
    &fontTex);
  
  offset_cursor = offset_font + grTexTextureMemRequired (GR_MIPMAPLEVELMASK_BOTH, &fontTex);
  
  free (fontTex.data);
  
  // ** Cursor texture **
  data = (DWORD*)cursor;
  
  WORD *tex16 = (WORD*)malloc(32*32*2);
  
  cursorTex.smallLodLog2 = cursorTex.largeLodLog2 = GR_LOD_LOG2_32;
  cursorTex.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
  cursorTex.format = GR_TEXFMT_ARGB_1555;
  cursorTex.data = tex16;
  
  // Conversion: [16-bit 1555 (swapped) --> 16-bit 1555]
  for (i=0; i<0x200; i++)
  {
    cur = *(data++);
    *(tex16++) = (WORD)(((cur&0x000000FF)<<8)|((cur&0x0000FF00)>>8));
    *(tex16++) = (WORD)(((cur&0x00FF0000)>>8)|((cur&0xFF000000)>>24));
  }
  
  grTexDownloadMipMap (GR_TMU0,
    grTexMinAddress(GR_TMU0) + offset_cursor,
    GR_MIPMAPLEVELMASK_BOTH,
    &cursorTex);
  
  // Round to higher 16
    offset_textures = ((offset_cursor + grTexTextureMemRequired (GR_MIPMAPLEVELMASK_BOTH, &cursorTex))
      & 0xFFFFFFF0) + 16;
  free (cursorTex.data);
}


BOOL InitGfx (BOOL evoodoo_using_window)
{
  if (fullscreen)
  {
    ReleaseGfx ();
  }
  
  OPEN_RDP_LOG ();  // doesn't matter if opens again; it will check for it
  OPEN_RDP_E_LOG ();
  LOG ("InitGfx ()\n");
  
  debugging = FALSE;
  
  // Initialize Glide
  grGlideInit ();
  
  // Select the Glide device
  grSstSelect (settings.card_id);
  
  gfx_context = 0;
  // Select the window

  if (settings.fb_hires)
  {
    printf("fb_hires\n");
    GRWINOPENEXT grSstWinOpenExt = (GRWINOPENEXT)grGetProcAddress("grSstWinOpenExt");
    if (grSstWinOpenExt)
      gfx_context = grSstWinOpenExt ((FxU32)gfx.hWnd,
      settings.res_data | ((evoodoo_using_window)?0x80:0x00),
      GR_REFRESH_60Hz,
      GR_COLORFORMAT_RGBA,
      GR_ORIGIN_UPPER_LEFT,
      GR_PIXFMT_RGB_565,
      2,    // Double-buffering
      1);   // 1 auxillary buffer
  }
  if (!gfx_context)
    gfx_context = grSstWinOpen ((FxU32)gfx.hWnd,
    settings.res_data | ((evoodoo_using_window)?0x80:0x00),
    GR_REFRESH_60Hz,
    GR_COLORFORMAT_RGBA,
    GR_ORIGIN_UPPER_LEFT,
    2,    // Double-buffering
    1);   // 1 auxillary buffer

  if (!gfx_context)
  {
    messagebox("Error", MB_OK|MB_ICONEXCLAMATION, "Error setting display mode");
    grSstWinClose (gfx_context);
    grGlideShutdown ();
    return FALSE;
  }
  
  // get the # of TMUs available
  grGet (GR_NUM_TMU, 4, (FxI32 *) &num_tmu);
  printf("num_tmu %d\n", num_tmu);
  // get maximal texture size
  grGet (GR_MAX_TEXTURE_SIZE, 4, (FxI32 *) &max_tex_size);
  //num_tmu = 1;
  
  // Is mirroring allowed?
  const char *extensions = grGetString (GR_EXTENSION);
  
  printf("bebefore\n");
  if (strstr (extensions, "TEXMIRROR"))
    sup_mirroring = 1;
  else
    sup_mirroring = 0;

  if (strstr (extensions, "TEXFMT"))  //VSA100 texture format extension
    sup_32bit_tex = TRUE;
  else
    sup_32bit_tex = FALSE;
  
  printf("bebefore2\n");
  if (settings.fb_hires)
  {
    if (char * extstr = strstr(extensions, "TEXTUREBUFFER")) 
    {
      if (!strncmp(extstr, "TEXTUREBUFFER", 13)) 
      {
        grTextureBufferExt = (GRTEXBUFFEREXT) grGetProcAddress("grTextureBufferExt");
        grTextureAuxBufferExt = (GRTEXBUFFEREXT) grGetProcAddress("grTextureAuxBufferExt");
        grAuxBufferExt = (GRAUXBUFFEREXT) grGetProcAddress("grAuxBufferExt");
      }
    }
    else 
      settings.fb_hires = 0;
  }
  else
    grTextureBufferExt = 0;

  grFramebufferCopyExt = (GRFRAMEBUFFERCOPYEXT) grGetProcAddress("grFramebufferCopyExt");
  printf("before\n");
  grStippleModeExt = (GRSTIPPLE) grStippleMode;
  grStipplePatternExt = (GRSTIPPLE) grStipplePattern; 
  printf("after\n");
  if (grStipplePatternExt)
    grStipplePatternExt(settings.stipple_pattern);

  InitCombine();

#ifdef SIMULATE_VOODOO1
  num_tmu = 1;
  sup_mirroring = 0;
#endif
  
#ifdef SIMULATE_BANSHEE
  num_tmu = 1;
  sup_mirroring = 1;
#endif
  
  fullscreen = TRUE;
  
  if (evoodoo_using_window)
    ev_fullscreen = FALSE;
  else
    ev_fullscreen = TRUE;

  grCoordinateSpace (GR_WINDOW_COORDS);
  grVertexLayout (GR_PARAM_XY, offsetof(VERTEX,x), GR_PARAM_ENABLE);
  grVertexLayout (GR_PARAM_Q, offsetof(VERTEX,q), GR_PARAM_ENABLE);
  grVertexLayout (GR_PARAM_Z, offsetof(VERTEX,z), GR_PARAM_ENABLE);
  grVertexLayout (GR_PARAM_ST0, offsetof(VERTEX,coord[0]), GR_PARAM_ENABLE);
  grVertexLayout (GR_PARAM_ST1, offsetof(VERTEX,coord[2]), GR_PARAM_ENABLE);
  grVertexLayout (GR_PARAM_PARGB, offsetof(VERTEX,b), GR_PARAM_ENABLE);
  
  grCullMode(GR_CULL_NEGATIVE);
  
  if (settings.fog) //"FOGCOORD" extension 
  {
    if (strstr (extensions, "FOGCOORD"))
    {
      GrFog_t fog_t[64];
      guFogGenerateLinear (fog_t, 0.0f, 255.0f);//(float)rdp.fog_multiplier + (float)rdp.fog_offset);//256.0f);
      
      for (int i = 63; i > 0; i--)
      {
        if (fog_t[i] - fog_t[i-1] > 63)
        {
          fog_t[i-1] = fog_t[i] - 63;
        }
      }
      fog_t[0] = 0;
      //      for (int f = 0; f < 64; f++)
      //      {
      //        FRDP("fog[%d]=%d->%f\n", f, fog_t[f], guFogTableIndexToW(f));
      //      }
      grFogTable (fog_t);
      grVertexLayout (GR_PARAM_FOG_EXT, offsetof(VERTEX,f), GR_PARAM_ENABLE);
    }
    else //not supported
      settings.fog = FALSE;
  }
  
  //grDepthBufferMode (GR_DEPTHBUFFER_WBUFFER);
  grDepthBufferMode (GR_DEPTHBUFFER_ZBUFFER);
  grDepthBufferFunction(GR_CMP_LESS);
  grDepthMask(FXTRUE);
  
  settings.res_x = settings.scr_res_x;
  settings.res_y = settings.scr_res_y;
  ChangeSize ();
  
  guLoadTextures ();
  grRenderBuffer(GR_BUFFER_BACKBUFFER);
  
  rdp_reset ();
  ClearCache ();
  
  rdp.update |= UPDATE_SCISSOR;
  
  return TRUE;
}

void ReleaseGfx ()
{
  // Release graphics
  grSstWinClose (gfx_context);
  
  // Shutdown glide
  grGlideShutdown();
  
  fullscreen = FALSE;
  rdp.window_changed = TRUE;
}

void CALL ReadScreen(void **dest, int *width, int *height)
{
  *width = settings.res_x;
  *height = settings.res_y;
  BYTE * buff = (BYTE *) malloc(settings.res_x * settings.res_y * 3);
  BYTE * line = buff;
  *dest = (void*)buff;
  
  if (!fullscreen)
  {
    for (DWORD y=0; y<settings.res_y; y++)
    {
      for (DWORD x=0; x<settings.res_x; x++)
      {
        line[x*3] = 0x20;
        line[x*3+1] = 0x7f;
        line[x*3+2] = 0x40;
      }
    }
    printf("[Glide64] Cannot save screenshot in windowed mode!\n");
    return;
  }
  
  GrLfbInfo_t info;
  info.size = sizeof(GrLfbInfo_t);
  if (grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER, GR_LFBWRITEMODE_888, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info))
  {
    // Copy the screen
    for (DWORD y=0; y<settings.res_y; y++)
    {
      BYTE *ptr = (BYTE*) info.lfbPtr + (info.strideInBytes * y);
      for (DWORD x=0; x<settings.res_x; x++)
      {
        line[x*3]   = ptr[2];  // red
        line[x*3+1] = ptr[1];  // green
        line[x*3+2] = ptr[0];  // blue
        ptr += 4;
      }
      line += settings.res_x * 3;
    }
    
    // Unlock the frontbuffer
    grLfbUnlock (GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER);
  }
  LOG ("ReadScreen. Success.\n");
}

/******************************************************************
Function: CaptureScreen
Purpose:  This function dumps the current frame to a file
input:    pointer to the directory to save the file to
output:   none
*******************************************************************/ 
EXPORT void CALL CaptureScreen ( char * Directory )
{
  capture_screen = 1;
  strcpy (capture_path, Directory);
}

/******************************************************************
Function: ChangeWindow
Purpose:  to change the window between fullscreen and window 
mode. If the window was in fullscreen this should 
change the screen to window mode and vice vesa.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL ChangeWindow (void)
{
  LOG ("ChangeWindow()\n");

  if (evoodoo)
  {
    if (!ev_fullscreen)
    {
      to_fullscreen = TRUE;
      GRWRAPPERFULLSCREENRESOLUTIONEXT grWrapperFullScreenResolutionExt = 
         (GRWRAPPERFULLSCREENRESOLUTIONEXT)grGetProcAddress("grWrapperFullScreenResolutionExt");
      if (grWrapperFullScreenResolutionExt != NULL)
      {
        settings.res_data_org = settings.res_data;
        settings.res_data = grWrapperFullScreenResolutionExt();
        settings.scr_res_x = settings.res_x = resolutions[settings.res_data][0];
        settings.scr_res_y = settings.res_y = resolutions[settings.res_data][1];
      }
    }
    else
    {
      ReleaseGfx ();
      GRWRAPPERFULLSCREENRESOLUTIONEXT grWrapperFullScreenResolutionExt = 
         (GRWRAPPERFULLSCREENRESOLUTIONEXT)grGetProcAddress("grWrapperFullScreenResolutionExt");
      if (grWrapperFullScreenResolutionExt != NULL)
      {
        settings.res_data = settings.res_data_org;
        settings.scr_res_x = settings.res_x = resolutions[settings.res_data][0];
        settings.scr_res_y = settings.res_y = resolutions[settings.res_data][1];
      }
      InitGfx(TRUE);
    }
  }
  else
  {
    // Go to fullscreen at next dlist
    // This is for compatibility with 1964, which reloads the plugin
    //  when switching to fullscreen
    if (!fullscreen)
    {
      to_fullscreen = TRUE;
    }
    else
    {
      ReleaseGfx ();
    }
  }
}

/******************************************************************
Function: CloseDLL
Purpose:  This function is called when the emulator is closing
down allowing the dll to de-initialise.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL CloseDLL (void)
{
  LOG ("CloseDLL ()\n");
  
  // re-set the old window proc
#ifdef WINPROC_OVERRIDE
  SetWindowLong (gfx.hWnd, GWL_WNDPROC, (long)oldWndProc);
#endif
  
#ifdef ALTTAB_FIX
  if (hhkLowLevelKybd)
  {
    UnhookWindowsHookEx(hhkLowLevelKybd);
    hhkLowLevelKybd = 0;
  }
#endif
  
  //CLOSELOG ();
  
  if (fullscreen)
    ReleaseGfx ();
  ZLUT_release();
  ClearCache ();
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
   messagebox("Glide64 v"G64_VERSION, MB_OK,
          "Glide64 "G64_VERSION"\nRelease: " RELTIME "\n" 
          "by GuentherB, Richard42, Gonetz, Dave2001, Gugaman, and others\n\n"
          "Beta testers: Raziel64, Federelli, Flash\n\n"
          "Special thanks to:\n"
          "Niki, FiRES, Icepir8, Rice, ZeZu, Azimer, Hacktarux, Cyberman, LoneRaven, Falcon4ever,\n"
          "GokuSS4, _Demo_, Ogy, Quvack, Scorpiove, CpUMasteR, Doom, Lemmy, CyRUS64,\n"
          "McLeod, Linker, StrmnNrmn, Tekken, ExtendedPlay, Kool Smoky\n"
          "everyone at EmuXHaven, all my testers, anyone I've forgotten, and anyone else on\n"
          "the Emutalk message board who helped or brought encouragement\n\n"
          "Thanks to EmuXHaven for hosting my site:\nhttp://glide64.emuxhaven.net/\n\n"
          "Official development channel: #Glide64 on EFnet\nNO ROM REQUESTS / NO BETA REQUESTS");
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
Function: DrawScreen
Purpose:  This function is called when the emulator receives a
WM_PAINT message. This allows the gfx to fit in when
it is being used in the desktop.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL DrawScreen (void)
{
  LOG ("DrawScreen ()\n");
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
  PluginInfo->Version = 0x0103;     // Set to 0x0103
  PluginInfo->Type  = PLUGIN_TYPE_GFX;  // Set to PLUGIN_TYPE_GFX
  sprintf (PluginInfo->Name, "Glide64 "G64_VERSION);  // Name of the DLL
  
  // If DLL supports memory these memory options then set them to TRUE or FALSE
  //  if it does not support it
  PluginInfo->NormalMemory = TRUE;  // a normal BYTE array
  PluginInfo->MemoryBswaped = TRUE; // a normal BYTE array where the memory has been pre
  // bswap on a dword (32 bits) boundry
}

BOOL WINAPI QueryPerformanceCounter(PLARGE_INTEGER counter)
{
   struct timeval tv;
   
   /* generic routine */
   gettimeofday( &tv, NULL );
   counter->QuadPart = (LONGLONG)tv.tv_usec + (LONGLONG)tv.tv_sec * 1000000;
   return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(PLARGE_INTEGER frequency)
{
   frequency->s.LowPart= 1000000;
   frequency->s.HighPart= 0;
   return TRUE;
}

/******************************************************************
Function: InitiateGFX
Purpose:  This function is called when the DLL is started to give
information from the emulator that the n64 graphics
uses. This is not called from the emulation thread.
Input:    Gfx_Info is passed to this function which is defined
above.
Output:   TRUE on success
FALSE on failure to initialise

  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/ 

EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{
  LOG ("InitiateGFX (*)\n");
  // Do *NOT* put this in rdp_reset or it could be set after the screen is initialized
  num_tmu = 2;
  
  // Assume scale of 1 for debug purposes
  rdp.scale_x = 1.0f;
  rdp.scale_y = 1.0f;
  
  memset (&settings, 0, sizeof(SETTINGS));
  ReadSettings ();
  
#ifdef FPS
  QueryPerformanceFrequency (&perf_freq);
  QueryPerformanceCounter (&fps_last);
#endif
  
  debug_init ();    // Initialize debugger
  
  gfx = Gfx_Info;
  /*  
  char name[21];
  // get the name of the ROM
  for (int i=0; i<20; i++)
    name[i] = gfx.HEADER[(32+i)^3];
  name[20] = 0;
  
  // remove all trailing spaces
  while (name[strlen(name)-1] == ' ')
    name[strlen(name)-1] = 0;
  
  ReadSpecialSettings (name);
  */  
#ifdef WINPROC_OVERRIDE
  if (!oldWndProc)
  {
    myWndProc = (WNDPROC)WndProc;
    oldWndProc = (WNDPROC)SetWindowLong (gfx.hWnd, GWL_WNDPROC, (long)myWndProc);
  }
#endif
  
  util_init ();
  math_init ();
  TexCacheInit ();
  CRC_BuildTable();
  CountCombine();
  if (settings.fb_depth_render)
    ZLUT_init();
  
  return TRUE;
}

/******************************************************************
Function: MoveScreen
Purpose:  This function is called in response to the emulator
receiving a WM_MOVE passing the xpos and ypos passed
from that message.
input:    xpos - the x-coordinate of the upper-left corner of the
client area of the window.
ypos - y-coordinate of the upper-left corner of the
client area of the window. 
output:   none
*******************************************************************/ 
EXPORT void CALL MoveScreen (int xpos, int ypos)
{
  LOG ("MoveScreen (" << xpos << ", " << ypos << ")\n");
}

/******************************************************************
Function: ProcessRDPList
Purpose:  This function is called when there is a Dlist to be
processed. (Low level GFX list)
input:    none
output:   none
*******************************************************************/
#if 0
EXPORT void CALL ProcessRDPList(void)
{
  if (settings.KI)
  {
    *gfx.MI_INTR_REG |= 0x20;
    gfx.CheckInterrupts();
  }
  LOG ("ProcessRDPList ()\n");
  printf("ProcessRPDList %x %x %x\n",
         *gfx.DPC_START_REG,
         *gfx.DPC_END_REG,
         *gfx.DPC_CURRENT_REG);
  //*gfx.DPC_STATUS_REG = 0xffffffff; // &= ~0x0002;

  //*gfx.DPC_START_REG = *gfx.DPC_END_REG;
  *gfx.DPC_CURRENT_REG = *gfx.DPC_END_REG;
}
#endif

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL RomClosed (void)
{
  LOG ("RomClosed ()\n");
  
  CLOSE_RDP_LOG ();
  CLOSE_RDP_E_LOG ();
  rdp.window_changed = TRUE;
  romopen = FALSE;
  if (fullscreen && evoodoo)
    ReleaseGfx ();
}

BOOL no_dlist = TRUE;

/******************************************************************
Function: RomOpen
Purpose:  This function is called when a rom is open. (from the 
emulation thread)
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL RomOpen (void)
{
  LOG ("RomOpen ()\n");
  no_dlist = TRUE;
  romopen = TRUE;
  ucode_error_report = TRUE;    // allowed to report ucode errors
  
  // Get the country code & translate to NTSC(0) or PAL(1)
  WORD code = ((WORD*)gfx.HEADER)[0x1F^1];
  
  if (code == 0x4400) region = 1; // Germany (PAL)
  if (code == 0x4500) region = 0; // USA (NTSC)
  if (code == 0x4A00) region = 0; // Japan (NTSC)
  if (code == 0x5000) region = 1; // Europe (PAL)
  if (code == 0x5500) region = 0; // Australia (NTSC)
  
  char name[21] = "DEFAULT";
  ReadSpecialSettings (name);
  
  // get the name of the ROM
  for (int i=0; i<20; i++)
    name[i] = gfx.HEADER[(32+i)^3];
  name[20] = 0;
  
  // remove all trailing spaces
  while (name[strlen(name)-1] == ' ')
    name[strlen(name)-1] = 0;
  
  ReadSpecialSettings (name);


  printf("fb_clear %d fb_smart %d\n", settings.fb_depth_clear, settings.fb_smart);

  
  rdp_reset ();
  ClearCache ();

  OPEN_RDP_LOG ();
  OPEN_RDP_E_LOG ();
  
  // ** EVOODOO EXTENSIONS **
  if (!fullscreen)
  {
    grGlideInit ();
    grSstSelect (0);
  }
  const char *extensions = grGetString (GR_EXTENSION);
  printf("extensions '%s'\n", extensions);
  if (!fullscreen)
  {
    grGlideShutdown ();
    
    if (strstr (extensions, "EVOODOO"))
      evoodoo = 1;
    else
      evoodoo = 0;
    
    if (evoodoo)
      InitGfx (TRUE);
  }
  
  if (strstr (extensions, "ROMNAME"))
  {
    void (__stdcall *grSetRomName)(char*);
    grSetRomName = (void (__stdcall *)(char*))grGetProcAddress ("grSetRomName");
    grSetRomName (name);
  }
  // **
}

/******************************************************************
Function: ShowCFB
Purpose:  Useally once Dlists are started being displayed, cfb is
ignored. This function tells the dll to start displaying
them again.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL ShowCFB (void)
{
  no_dlist = TRUE;
  LOG ("ShowCFB ()\n");
}


void drawViRegBG();
void drawNoFullscreenMessage();

void DrawFrameBuffer ()
  {
    if (!fullscreen)
    {
      drawNoFullscreenMessage();
    }
    if (to_fullscreen)
    {
      to_fullscreen = FALSE;
      
      if (!InitGfx (FALSE))
      {
        LOG ("FAILED!!!\n");
        return;
      }
      fullscreen = TRUE;
    }
    
    if (fullscreen)
    {
      grDepthMask (FXTRUE);
      grColorMask (FXTRUE, FXTRUE);
      grBufferClear (0, 0, 0xFFFF);
      drawViRegBG();
    }
}

EXPORT void CALL SetRenderingCallback(void (*callback)())
{
    renderCallback = callback;
}

/******************************************************************
Function: UpdateScreen
Purpose:  This function is called in response to a vsync of the
screen were the VI bit in MI_INTR_REG has already been
set
input:    none
output:   none
*******************************************************************/ 
DWORD update_screen_count = 0;
EXPORT void CALL UpdateScreen (void)
{
#ifdef LOG_KEY
  if (GetAsyncKeyState (VK_SPACE) & 0x0001)
  {
    LOG ("KEY!!!\n");
  }
#endif
  char out_buf[512];
  sprintf (out_buf, "UpdateScreen (). distance: %d\n", (int)(*gfx.VI_ORIGIN_REG) - (int)((*gfx.VI_WIDTH_REG) << 2));
  LOG (out_buf);
  //  LOG ("UpdateScreen ()\n");
  
  DWORD width = (*gfx.VI_WIDTH_REG) << 1;
  if (fullscreen && (*gfx.VI_ORIGIN_REG  > width))
    update_screen_count++;
  
  // vertical interrupt has occured, increment counter
  vi_count ++;
  
#ifdef FPS
  // Check frames per second
  LARGE_INTEGER difference;
  QueryPerformanceCounter (&fps_next);
  difference.QuadPart = fps_next.QuadPart - fps_last.QuadPart;
  float diff_secs = (float)((double)difference.QuadPart / (double)perf_freq.QuadPart);
  if (diff_secs > 0.5f)
  {
    fps = (float)fps_count / diff_secs;
    vi = (float)vi_count / diff_secs;
    ntsc_percent = vi / 0.6f;
    pal_percent = vi / 0.5f;
    fps_last = fps_next;
    fps_count = 0;
    vi_count = 0;
  }
#endif
  //*  
  DWORD limit = settings.lego ? 15 : 50;
  if (settings.cpu_write_hack && (update_screen_count > limit) && (rdp.last_bg == 0)) 
  {
    RDP("DirectCPUWrite hack!\n");
    update_screen_count = 0;
    no_dlist = TRUE;
    ClearCache ();
    UpdateScreen();
    return;
  }
  //*/
  //*
  if( no_dlist )
  {
    if( *gfx.VI_ORIGIN_REG  > width )
    {
      ChangeSize ();
      RDP("ChangeSize done\n");
      DrawFrameBuffer();
      RDP("DrawFrameBuffer done\n");
      rdp.updatescreen = 1;
      newSwapBuffers ();
    }
    return;
  }
  //*/  
  if (settings.swapmode == 0)
  {
    newSwapBuffers ();
  }
}

DWORD curframe = 0;
void newSwapBuffers()
{
  if (rdp.updatescreen)
  {
    rdp.updatescreen = 0;
    
    RDP ("swapped\n");
 
    // Allow access to the whole screen
    if (fullscreen)
    {
      grClipWindow (0, 0, settings.scr_res_x, settings.scr_res_y);
      grDepthBufferFunction (GR_CMP_ALWAYS);
      grDepthMask (FXFALSE);
      
      grCullMode (GR_CULL_DISABLE);

      if ((settings.show_fps & 0xF) || settings.clock)
        set_message_combiner ();
#ifdef FPS
      float y = (float)settings.res_y;
      if (settings.show_fps & 0x0F)
      {
        if (settings.show_fps & 4)
        {
          if (region)   // PAL
            output (0, y, 0, "%d%% ", (int)pal_percent);
          else
            output (0, y, 0, "%d%% ", (int)ntsc_percent);
          y -= 16;
        }
        if (settings.show_fps & 2)
        {
          output (0, y, 0, "VI/s: %.02f ", vi);
          y -= 16;
        }
        if (settings.show_fps & 1)
          output (0, y, 0, "FPS: %.02f ", fps);
      }
#endif
      
      if (settings.clock)
      {
        if (settings.clock_24_hr)
        {
          time_t ltime;
          time (&ltime);
          tm *cur_time = localtime (&ltime);
          
          sprintf (out_buf, "%.2d:%.2d:%.2d", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
        }
        else
        {
          char ampm[] = "AM";
          time_t ltime;
          
          time (&ltime);
          tm *cur_time = localtime (&ltime);
          
          if (cur_time->tm_hour >= 12)
          {
            strcpy (ampm, "PM");
            if (cur_time->tm_hour != 12)
              cur_time->tm_hour -= 12;
          }
          if (cur_time->tm_hour == 0)
            cur_time->tm_hour = 12;
          
          if (cur_time->tm_hour >= 10)
            sprintf (out_buf, "%.5s %s", asctime(cur_time) + 11, ampm);
          else
            sprintf (out_buf, " %.4s %s", asctime(cur_time) + 12, ampm);
        }
        output ((float)(settings.res_x - 68), y, 0, out_buf, 0);
      }
    }

    // Capture the screen if debug capture is set
    if (debug.capture)
    {
      // Allocate the screen
      debug.screen = new BYTE [(settings.res_x*settings.res_y) << 1];
      
      // Lock the backbuffer (already rendered)
      GrLfbInfo_t info;
      info.size = sizeof(GrLfbInfo_t);
      while (!grLfbLock (GR_LFB_READ_ONLY,
        GR_BUFFER_BACKBUFFER,
        GR_LFBWRITEMODE_565,
        GR_ORIGIN_UPPER_LEFT,
        FXFALSE,
        &info));
      
      DWORD offset_src=0/*(settings.scr_res_y-settings.res_y)*info.strideInBytes*/, offset_dst=0;
      
      // Copy the screen
      for (DWORD y=0; y<settings.res_y; y++)
      {
        memcpy (debug.screen + offset_dst, (BYTE*)info.lfbPtr + offset_src, settings.res_x << 1);
        offset_dst += settings.res_x << 1;
        offset_src += info.strideInBytes;
      }
      
      // Unlock the backbuffer
      grLfbUnlock (GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER);
    }

    if (fullscreen)
    {
      LOG ("BUFFER SWAPPED\n"); 
      grBufferSwap (settings.vsync);    
      fps_count ++;
    }
    
    if (fullscreen && (debugging || settings.wireframe || settings.buff_clear))
    {
      if (settings.RE2 && settings.fb_depth_render)
        grDepthMask (FXFALSE);
      else
        grDepthMask (FXTRUE);
      grBufferClear (0, 0, 0xFFFF);
    }
    
    frame_count ++;
  }
}

/******************************************************************
Function: ViStatusChanged
Purpose:  This function is called to notify the dll that the
ViStatus registers value has been changed.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL ViStatusChanged (void)
{
}

/******************************************************************
Function: ViWidthChanged
Purpose:  This function is called to notify the dll that the
ViWidth registers value has been changed.
input:    none
output:   none
*******************************************************************/ 
EXPORT void CALL ViWidthChanged (void)
{
}

#ifdef WINPROC_OVERRIDE
LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_ACTIVATE:
    rdp.window_changed = TRUE;
    break;
    
    /*    case WM_DESTROY:
    SetWindowLong (gfx.hWnd, GWL_WNDPROC, (long)oldWndProc);
    break;*/
  }
  
  return CallWindowProc(oldWndProc, hwnd, msg, wParam, lParam);
}
#endif

BOOL k_ctl=0, k_alt=0, k_del=0;

#ifdef ALTTAB_FIX
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, 
                                      WPARAM wParam, LPARAM lParam) {
  if (!fullscreen) return CallNextHookEx(NULL, nCode, wParam, lParam);
  
  BOOL TabKey = FALSE;
  
  PKBDLLHOOKSTRUCT p;
  
  if (nCode == HC_ACTION) 
  {
    switch (wParam) {
    case WM_KEYUP:    case WM_SYSKEYUP:
      p = (PKBDLLHOOKSTRUCT) lParam;
      if (p->vkCode == 162) k_ctl = 0;
      if (p->vkCode == 164) k_alt = 0;
      if (p->vkCode == 46) k_del = 0;
      goto do_it;
      
    case WM_KEYDOWN:  case WM_SYSKEYDOWN:
      p = (PKBDLLHOOKSTRUCT) lParam;
      if (p->vkCode == 162) k_ctl = 1;
      if (p->vkCode == 164) k_alt = 1;
      if (p->vkCode == 46) k_del = 1;
      goto do_it;
      
do_it:    
      TabKey =
        ((p->vkCode == VK_TAB) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
        ((p->vkCode == VK_ESCAPE) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
        ((p->vkCode == VK_ESCAPE) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0)) ||
        (k_ctl && k_alt && k_del);
      
      break;
    }
  }
  
  if (TabKey)
  {
    k_ctl = 0;
    k_alt = 0;
    k_del = 0;
    ReleaseGfx ();
  }
  
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

