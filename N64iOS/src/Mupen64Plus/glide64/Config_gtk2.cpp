/*
*   Glide64 - Glide video plugin for Nintendo 64 emulators.
*   Copyright (c) 2002  Dave2001
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
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

#include <string.h>
#include <gtk/gtk.h>

typedef struct
{
   GtkWidget *dialog;
   GtkWidget *autodetectCheckButton;
   GtkWidget *forceCombo;
   GtkWidget *windowResCombo;
   GtkWidget *fullResCombo;
   GtkWidget *texFilterCombo;
   GtkWidget *filterCombo;
   GtkWidget *lodCombo;
   GtkWidget *fogCheckButton;
   GtkWidget *bufferClearCheckButton;
   GtkWidget *vSyncCheckButton;
   GtkWidget *fastcrcCheckButton;
   GtkWidget *noDitheredAlphaCheckButton;
   GtkWidget *noGLSLCheckButton;
   GtkWidget *swapCombo;
   GtkWidget *customIniCheckButton;
   GtkWidget *wrapCheckButton;
   GtkWidget *coronaCheckButton;
   GtkWidget *readAllCheckButton;
   GtkWidget *CPUWriteHackCheckButton;
   GtkWidget *FBGetInfoCheckButton;
   GtkWidget *DepthRenderCheckButton;
   GtkWidget *FPSCheckButton;
   GtkWidget *VICheckButton;
   GtkWidget *ratioCheckButton;
   GtkWidget *FPStransCheckButton;
   GtkWidget *clockCheckButton;
   GtkWidget *clock24CheckButton;
   GtkWidget *hiresFbCheckButton;
   GtkWidget *hiresFBOCheckButton;
   GList *windowResComboList;
   GList *texFilterComboList;
   GList *forceComboList;
   GList *filterComboList;
   GList *lodComboList;
   GList *swapComboList;
} ConfigDialog;

static void customIniCheckButtonCallback(GtkWidget *widget, void *data)
{
   ConfigDialog *configDialog = (ConfigDialog*)data;
   
   BOOL enable = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->customIniCheckButton));
   gtk_widget_set_sensitive(configDialog->filterCombo, enable);
   gtk_widget_set_sensitive(configDialog->lodCombo, enable);
   gtk_widget_set_sensitive(configDialog->fogCheckButton, enable);
   gtk_widget_set_sensitive(configDialog->bufferClearCheckButton, enable);
   gtk_widget_set_sensitive(configDialog->swapCombo, enable);
}

static void okButtonCallback(GtkWidget *widget, void *data)
{
   ConfigDialog *configDialog = (ConfigDialog*)data;
   char *s;
   unsigned int i;
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->windowResCombo)->entry));
   for (i=0; i<g_list_length(configDialog->windowResComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->windowResComboList, i)))
       settings.res_data = i;
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->fullResCombo)->entry));
   for (i=0; i<g_list_length(configDialog->windowResComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->windowResComboList, i)))
       settings.full_res = i;
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->texFilterCombo)->entry));
   for (i=0; i<g_list_length(configDialog->texFilterComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->texFilterComboList, i)))
       settings.tex_filter = i;
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->forceCombo)->entry));
   for (i=0; i<g_list_length(configDialog->forceComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->forceComboList, i)))
       settings.ucode = i;
   
   settings.scr_res_x = settings.res_x = resolutions[settings.res_data][0];
   settings.scr_res_y = settings.res_y = resolutions[settings.res_data][1];
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->filterCombo)->entry));
   for (i=0; i<g_list_length(configDialog->filterComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->filterComboList, i)))
       settings.filtering = i;

   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->lodCombo)->entry));
   for (i=0; i<g_list_length(configDialog->lodComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->lodComboList, i)))
       settings.lodmode = i;
   
   s = (char*)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(configDialog->swapCombo)->entry));
   for (i=0; i<g_list_length(configDialog->swapComboList); i++)
     if(!strcmp(s, (char*)g_list_nth_data(configDialog->swapComboList, i)))
       settings.swapmode = i;
   
   settings.fog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->fogCheckButton));
   settings.buff_clear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->bufferClearCheckButton));
   settings.autodetect_ucode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->autodetectCheckButton));
   settings.wrap_big_tex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->wrapCheckButton));
   settings.flame_corona = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->coronaCheckButton));
   settings.vsync = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->vSyncCheckButton));
   settings.fast_crc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->fastcrcCheckButton));
   settings.noditheredalpha = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->noDitheredAlphaCheckButton));
   settings.noglsl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->noGLSLCheckButton));
   settings.fb_read_always = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->readAllCheckButton));
   settings.cpu_write_hack = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->CPUWriteHackCheckButton));
   settings.fb_get_info = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->FBGetInfoCheckButton));
   settings.fb_depth_render = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->DepthRenderCheckButton));
   settings.custom_ini = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->customIniCheckButton));
   settings.fb_hires = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->hiresFbCheckButton));
   settings.FBO = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->hiresFBOCheckButton));
   
   settings.show_fps =
     (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->FPSCheckButton))?1:0) |
     (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->VICheckButton))?2:0) |
     (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->ratioCheckButton))?4:0) |
     (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->FPStransCheckButton))?8:0);
   
   settings.clock = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->clockCheckButton));
   settings.clock_24_hr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialog->clock24CheckButton));
   
   WriteSettings();
   
   // re-init evoodoo graphics to resize window
   if (evoodoo && fullscreen && !ev_fullscreen) {
      ReleaseGfx ();
      InitGfx (TRUE);
   }
   
   gtk_widget_hide(configDialog->dialog);
}

static ConfigDialog *CreateConfigDialog()
{
   // objects dialog
   // dialog
   GtkWidget *dialog;
   dialog = gtk_dialog_new();
   gtk_window_set_title(GTK_WINDOW(dialog), "Glide64 Configuration");
   
   // ok button
   GtkWidget *okButton;
   okButton = gtk_button_new_with_label("OK");
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), okButton);
   
   // cancel button
   GtkWidget *cancelButton;
   cancelButton = gtk_button_new_with_label("Cancel");
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), cancelButton);
   
   // Autodetect ucode CheckButton
   GtkWidget *autodetectCheckButton;
   autodetectCheckButton = gtk_check_button_new_with_label("Autodetect Microcode");
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), autodetectCheckButton);
   
   // Force Microcode Container
   GtkWidget *forceContainer;
   forceContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), forceContainer);
   
   // Force Microcode Label
   GtkWidget *forceLabel;
   forceLabel = gtk_label_new("Force Microcode:");
   gtk_container_add(GTK_CONTAINER(forceContainer), forceLabel);
   
   // Force Microcode Combo
   GtkWidget *forceCombo;
   forceCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(forceCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(forceContainer), forceCombo);
   
   // horizontal container
   GtkWidget *hContainer;
   hContainer = gtk_hbox_new(0, 0);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hContainer);
   
   // vertical container
   GtkWidget *vContainer;
   vContainer = gtk_vbox_new(0, 0);
   gtk_container_add(GTK_CONTAINER(hContainer), vContainer);
   
   // Rendering Frame
   GtkWidget *renderingFrame;
   renderingFrame = gtk_frame_new("Rendering");
   gtk_container_add(GTK_CONTAINER(vContainer), renderingFrame);

   // Rendering Container
   GtkWidget *renderingContainer;
   renderingContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingFrame), renderingContainer);
   
   // Window Mode Resolution Container
   GtkWidget *windowResContainer;
   windowResContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), windowResContainer);
   
   // Window Mode Resolution Label
   GtkWidget *windowResLabel;
   windowResLabel = gtk_label_new("Window Resolution:");
   gtk_container_add(GTK_CONTAINER(windowResContainer), windowResLabel);
   
   // Window Mode Combo
   GtkWidget *windowResCombo;
   windowResCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(windowResCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(windowResContainer), windowResCombo);
   
   // FullScreen Mode Resolution Container
   GtkWidget *fullResContainer;
   fullResContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), fullResContainer);
   
   // FullScreen Mode Resolution Label
   GtkWidget *fullResLabel;
   fullResLabel = gtk_label_new("Fullscreen Resolution:");
   gtk_container_add(GTK_CONTAINER(fullResContainer), fullResLabel);
   
   // FullScreen Mode Combo
   GtkWidget *fullResCombo;
   fullResCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fullResCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(fullResContainer), fullResCombo);
   
   // Texture Filter Container
   GtkWidget *texFilterContainer;
   texFilterContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), texFilterContainer);
   
   // Texture Filter Label
   GtkWidget *texFilterLabel;
   texFilterLabel = gtk_label_new("Texture Filter:");
   gtk_container_add(GTK_CONTAINER(texFilterContainer), texFilterLabel);
   
   // Texture Filter Combo
   GtkWidget *texFilterCombo;
   texFilterCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(texFilterCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(texFilterContainer), texFilterCombo);
   
   // Filter Container
   GtkWidget *filterContainer;
   filterContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), filterContainer);
   
   // Filter Label
   GtkWidget *filterLabel;
   filterLabel = gtk_label_new("Filtering mode:");
   gtk_container_add(GTK_CONTAINER(filterContainer), filterLabel);
   
   // Filter Combo
   GtkWidget *filterCombo;
   filterCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(filterCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(filterContainer), filterCombo);
   
   // LOD Container
   GtkWidget *lodContainer;
   lodContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), lodContainer);
   
   // LOD Label
   GtkWidget *lodLabel;
   lodLabel = gtk_label_new("LOD calculation:");
   gtk_container_add(GTK_CONTAINER(lodContainer), lodLabel);
   
   // LOD Combo
   GtkWidget *lodCombo;
   lodCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(lodCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(lodContainer), lodCombo);
   
   // Fog CheckButton
   GtkWidget *fogCheckButton;
   fogCheckButton = gtk_check_button_new_with_label("Fog enabled");
   gtk_container_add(GTK_CONTAINER(renderingContainer), fogCheckButton);
   
   // Buffer Clear CheckButton
   GtkWidget *bufferClearCheckButton;
   bufferClearCheckButton = gtk_check_button_new_with_label("Buffer clear on every frame");
   gtk_container_add(GTK_CONTAINER(renderingContainer), bufferClearCheckButton);
   
   // Vertical Sync CheckButton
   GtkWidget *vSyncCheckButton;
   vSyncCheckButton = gtk_check_button_new_with_label("Vertical Sync");
   gtk_container_add(GTK_CONTAINER(renderingContainer), vSyncCheckButton);
   
   // Fast CRC CheckButton
   GtkWidget *fastcrcCheckButton;
   fastcrcCheckButton = gtk_check_button_new_with_label("Fast CRC");
   gtk_container_add(GTK_CONTAINER(renderingContainer), fastcrcCheckButton);
   
   // hires framebuffer CheckButton
   GtkWidget *hiresFbCheckButton;
   hiresFbCheckButton = gtk_check_button_new_with_label("Hires Framebuffer");
   gtk_container_add(GTK_CONTAINER(renderingContainer), hiresFbCheckButton);
   
   // Swap Container
   GtkWidget *swapContainer;
   swapContainer = gtk_hbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(renderingContainer), swapContainer);
   
   // Swap Combo
   GtkWidget *swapCombo;
   swapCombo = gtk_combo_new();
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(swapCombo)->entry), FALSE);
   gtk_container_add(GTK_CONTAINER(swapContainer), swapCombo);
   
   // Swap Label
   GtkWidget *swapLabel;
   swapLabel = gtk_label_new("Buffer swapping method");
   gtk_container_add(GTK_CONTAINER(swapContainer), swapLabel);
   
   // Rendering Frame
   GtkWidget *wrapperFrame;
   wrapperFrame = gtk_frame_new("Glide wrapper");
   gtk_container_add(GTK_CONTAINER(vContainer), wrapperFrame);

   // Wrapper Container
   GtkWidget *wrapperContainer;
   wrapperContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(wrapperFrame), wrapperContainer);
   
   // no dithered alpha CheckButton
   GtkWidget *noDitheredAlphaCheckButton;
   noDitheredAlphaCheckButton = gtk_check_button_new_with_label("Disable Dithered Alpha");
   gtk_container_add(GTK_CONTAINER(wrapperContainer), noDitheredAlphaCheckButton);

   // no glsl CheckButton
   GtkWidget *noGLSLCheckButton;
   noGLSLCheckButton = gtk_check_button_new_with_label("Disable GLSL Combiners");
   gtk_container_add(GTK_CONTAINER(wrapperContainer), noGLSLCheckButton);
   
   // FBO CheckButton
   GtkWidget *hiresFBOCheckButton;
   hiresFBOCheckButton = gtk_check_button_new_with_label("Use Framebuffer Objects");
   gtk_container_add(GTK_CONTAINER(wrapperContainer), hiresFBOCheckButton);
   
   // Other Frame
   GtkWidget *otherFrame;
   otherFrame = gtk_frame_new("Other");
   gtk_container_add(GTK_CONTAINER(vContainer), otherFrame);
   
   // Custom ini CheckButton
   GtkWidget *customIniCheckButton;
   customIniCheckButton = gtk_check_button_new_with_label("Custom ini settings");
   gtk_container_add(GTK_CONTAINER(otherFrame), customIniCheckButton);
   
   // vertical container
   vContainer = gtk_vbox_new(0, 0);
   gtk_container_add(GTK_CONTAINER(hContainer), vContainer);
   
   // Special Fixes Frame
   GtkWidget *specialFrame;
   specialFrame = gtk_frame_new("Special Fixes");
   gtk_container_add(GTK_CONTAINER(vContainer), specialFrame);
   
   // Special Fixes Container
   GtkWidget *specialContainer;
   specialContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(specialFrame), specialContainer);
   
   // Wrap CheckButton
   GtkWidget *wrapCheckButton;
   wrapCheckButton = gtk_check_button_new_with_label("Wrap textures too big for tmem");
   gtk_container_add(GTK_CONTAINER(specialContainer), wrapCheckButton);
   
   // Corona CheckButton
   GtkWidget *coronaCheckButton;
   coronaCheckButton = gtk_check_button_new_with_label("Zelda. Corona fix");
   gtk_container_add(GTK_CONTAINER(specialContainer), coronaCheckButton);
   
   // Frame Buffer Frame
   GtkWidget *framebufferFrame;
   framebufferFrame = gtk_frame_new("Frame buffer emulation options");
   gtk_container_add(GTK_CONTAINER(vContainer), framebufferFrame);
   
   // Frame Buffer Container
   GtkWidget *framebufferContainer;
   framebufferContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(framebufferFrame), framebufferContainer);
   
   // Read All CheckButton
   GtkWidget *readAllCheckButton;
   readAllCheckButton = gtk_check_button_new_with_label("Read every frame (slow!)");
   gtk_container_add(GTK_CONTAINER(framebufferContainer), readAllCheckButton);
   
   // CPU Write Hack CheckButton
   GtkWidget *CPUWriteHackCheckButton;
   CPUWriteHackCheckButton = gtk_check_button_new_with_label("Detect CPU writes");
   gtk_container_add(GTK_CONTAINER(framebufferContainer), CPUWriteHackCheckButton);
   
   // FB Get Info CheckButton
   GtkWidget *FBGetInfoCheckButton;
   FBGetInfoCheckButton = gtk_check_button_new_with_label("Get frame buffer info");
   gtk_container_add(GTK_CONTAINER(framebufferContainer), FBGetInfoCheckButton);
   
   // Depth Render CheckButton
   GtkWidget *DepthRenderCheckButton;
   DepthRenderCheckButton = gtk_check_button_new_with_label("Depth buffer render");
   gtk_container_add(GTK_CONTAINER(framebufferContainer), DepthRenderCheckButton);
   
   // Speed Frame
   GtkWidget *speedFrame;
   speedFrame = gtk_frame_new("Speed");
   gtk_container_add(GTK_CONTAINER(vContainer), speedFrame);
   
   // Speed Container
   GtkWidget *speedContainer;
   speedContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(speedFrame), speedContainer);
   
   // FPS CheckButton
   GtkWidget *FPSCheckButton;
   FPSCheckButton = gtk_check_button_new_with_label("FPS counter");
   gtk_container_add(GTK_CONTAINER(speedContainer), FPSCheckButton);
   
   // VI CheckButton
   GtkWidget *VICheckButton;
   VICheckButton = gtk_check_button_new_with_label("VI/s counter");
   gtk_container_add(GTK_CONTAINER(speedContainer), VICheckButton);
   
   // ratio CheckButton
   GtkWidget *ratioCheckButton;
   ratioCheckButton = gtk_check_button_new_with_label("% speed");
   gtk_container_add(GTK_CONTAINER(speedContainer), ratioCheckButton);
   
   // FPS trans CheckButton
   GtkWidget *FPStransCheckButton;
   FPStransCheckButton = gtk_check_button_new_with_label("FPS transparent");
   gtk_container_add(GTK_CONTAINER(speedContainer), FPStransCheckButton);
   
   // Time Frame
   GtkWidget *timeFrame;
   timeFrame = gtk_frame_new("Time");
   gtk_container_add(GTK_CONTAINER(vContainer), timeFrame);
   
   // Time Container
   GtkWidget *timeContainer;
   timeContainer = gtk_vbox_new(TRUE, 0);
   gtk_container_add(GTK_CONTAINER(timeFrame), timeContainer);
   
   // clock CheckButton
   GtkWidget *clockCheckButton;
   clockCheckButton = gtk_check_button_new_with_label("Clock enabled");
   gtk_container_add(GTK_CONTAINER(timeContainer), clockCheckButton);
   
   // clock24 CheckButton
   GtkWidget *clock24CheckButton;
   clock24CheckButton = gtk_check_button_new_with_label("Clock is 24-hour");
   gtk_container_add(GTK_CONTAINER(timeContainer), clock24CheckButton);
   
   // Filling lists
   // windowResCombo list
   GList *windowResComboList = NULL;
   windowResComboList = g_list_append(windowResComboList, (void*)"320x200");
   windowResComboList = g_list_append(windowResComboList, (void*)"320x240");
   windowResComboList = g_list_append(windowResComboList, (void*)"400x256");
   windowResComboList = g_list_append(windowResComboList, (void*)"512x384");
   windowResComboList = g_list_append(windowResComboList, (void*)"640x200");
   windowResComboList = g_list_append(windowResComboList, (void*)"640x350");
   windowResComboList = g_list_append(windowResComboList, (void*)"640x400");
   windowResComboList = g_list_append(windowResComboList, (void*)"640x480");
   windowResComboList = g_list_append(windowResComboList, (void*)"800x600");
   windowResComboList = g_list_append(windowResComboList, (void*)"960x720");
   windowResComboList = g_list_append(windowResComboList, (void*)"856x480");
   windowResComboList = g_list_append(windowResComboList, (void*)"512x256");
   windowResComboList = g_list_append(windowResComboList, (void*)"1024x768");
   windowResComboList = g_list_append(windowResComboList, (void*)"1280x1024");
   windowResComboList = g_list_append(windowResComboList, (void*)"1600x1200");
   windowResComboList = g_list_append(windowResComboList, (void*)"400x300");
   windowResComboList = g_list_append(windowResComboList, (void*)"1152x864");
   windowResComboList = g_list_append(windowResComboList, (void*)"1280x960");
   windowResComboList = g_list_append(windowResComboList, (void*)"1600x1024");
   windowResComboList = g_list_append(windowResComboList, (void*)"1792x1344");
   windowResComboList = g_list_append(windowResComboList, (void*)"1856x1392");
   windowResComboList = g_list_append(windowResComboList, (void*)"1920x1440");
   windowResComboList = g_list_append(windowResComboList, (void*)"2048x1536");
   windowResComboList = g_list_append(windowResComboList, (void*)"2048x2048");
   gtk_combo_set_popdown_strings(GTK_COMBO(windowResCombo),
                 windowResComboList);
   
   // fullResCombo list
   gtk_combo_set_popdown_strings(GTK_COMBO(fullResCombo), windowResComboList);
   
   // texFilterCombo list
   GList *texFilterComboList = NULL;
   texFilterComboList = g_list_append(texFilterComboList, (void*)"None");
   texFilterComboList = g_list_append(texFilterComboList, (void*)"Blur edges");
   texFilterComboList = g_list_append(texFilterComboList, (void*)"Super 2xSai");
   texFilterComboList = g_list_append(texFilterComboList, (void*)"Hq2x");
   texFilterComboList = g_list_append(texFilterComboList, (void*)"Hq4x");
   gtk_combo_set_popdown_strings(GTK_COMBO(texFilterCombo), 
                 texFilterComboList);
   
   // forceCombo list
   GList *forceComboList = NULL;
   forceComboList = g_list_append(forceComboList, (void*)"0: RSP SW 2.0X (ex. Mario)");
   forceComboList = g_list_append(forceComboList, (void*)"1: F3DEX 1.XX (ex. Star Fox)");
   forceComboList = g_list_append(forceComboList, (void*)"2: F3DEX 2.XX (ex. Zelda OOT)");
   forceComboList = g_list_append(forceComboList, (void*)"3: RSP SW 2.0D EXT (ex. Waverace)");
   forceComboList = g_list_append(forceComboList, (void*)"4: RSP SW 2.0D EXT (ex. Shadows of the Empire)");
   forceComboList = g_list_append(forceComboList, (void*)"5: RSP SW 2.0 (ex. Diddy Kong Racing)");
   forceComboList = g_list_append(forceComboList, (void*)"6: S2DEX 1.XX (ex. Yoshi's Story)");
   forceComboList = g_list_append(forceComboList, (void*)"7: RSP SW PD Perfect Dark");
   forceComboList = g_list_append(forceComboList, (void*)"8: F3DEXBG 2.08 Conker's Bad Fur Day");
   gtk_combo_set_popdown_strings(GTK_COMBO(forceCombo), forceComboList);
   
   // filterCombo list
   GList *filterComboList = NULL;
   filterComboList = g_list_append(filterComboList, (void*)"Automatic");
   filterComboList = g_list_append(filterComboList, (void*)"Force Bilinear");
   filterComboList = g_list_append(filterComboList, (void*)"Force Point-sampled");
   gtk_combo_set_popdown_strings(GTK_COMBO(filterCombo), filterComboList);
   
   // lodCombo list
   GList *lodComboList = NULL;
   lodComboList = g_list_append(lodComboList, (void*)"Off");
   lodComboList = g_list_append(lodComboList, (void*)"Fast");
   lodComboList = g_list_append(lodComboList, (void*)"Precise");
   gtk_combo_set_popdown_strings(GTK_COMBO(lodCombo), lodComboList);
   
   // swapCombo list
   GList *swapComboList = NULL;
   swapComboList = g_list_append(swapComboList, (void*)"old");
   swapComboList = g_list_append(swapComboList, (void*)"new");
   swapComboList = g_list_append(swapComboList, (void*)"hybrid");
   gtk_combo_set_popdown_strings(GTK_COMBO(swapCombo), swapComboList);
   
   // ConfigDialog structure creation
   ConfigDialog *configDialog = new ConfigDialog;
   
   // signal callbacks
   gtk_signal_connect_object(GTK_OBJECT(dialog), "delete-event",
                 GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete),
                 GTK_OBJECT(dialog));
   gtk_signal_connect(GTK_OBJECT(customIniCheckButton), "clicked",
              GTK_SIGNAL_FUNC(customIniCheckButtonCallback),
              (void*)configDialog);
   gtk_signal_connect(GTK_OBJECT(okButton), "clicked",
              GTK_SIGNAL_FUNC(okButtonCallback),
              (void*)configDialog);
   gtk_signal_connect_object(GTK_OBJECT(cancelButton), "clicked",
                 GTK_SIGNAL_FUNC(gtk_widget_hide),
                 GTK_OBJECT(dialog));
              
   // Outputing ConfigDialog structure
   configDialog->dialog = dialog;
   configDialog->autodetectCheckButton = autodetectCheckButton;
   configDialog->forceCombo = forceCombo;
   configDialog->windowResCombo = windowResCombo;
   configDialog->fullResCombo = fullResCombo;
   configDialog->texFilterCombo = texFilterCombo;
   configDialog->filterCombo = filterCombo;
   configDialog->lodCombo = lodCombo;
   configDialog->fogCheckButton = fogCheckButton;
   configDialog->bufferClearCheckButton = bufferClearCheckButton;
   configDialog->vSyncCheckButton = vSyncCheckButton;
   configDialog->fastcrcCheckButton = fastcrcCheckButton;
   configDialog->noDitheredAlphaCheckButton = noDitheredAlphaCheckButton;
   configDialog->noGLSLCheckButton = noGLSLCheckButton;
   configDialog->hiresFbCheckButton = hiresFbCheckButton;
   configDialog->hiresFBOCheckButton = hiresFBOCheckButton;
   configDialog->swapCombo = swapCombo;
   configDialog->customIniCheckButton = customIniCheckButton;
   configDialog->wrapCheckButton = wrapCheckButton;
   configDialog->coronaCheckButton = coronaCheckButton;
   configDialog->readAllCheckButton = readAllCheckButton;
   configDialog->CPUWriteHackCheckButton = CPUWriteHackCheckButton;
   configDialog->FBGetInfoCheckButton = FBGetInfoCheckButton;
   configDialog->DepthRenderCheckButton = DepthRenderCheckButton;
   configDialog->FPSCheckButton = FPSCheckButton;
   configDialog->VICheckButton = VICheckButton;
   configDialog->ratioCheckButton = ratioCheckButton;
   configDialog->FPStransCheckButton = FPStransCheckButton;
   configDialog->clockCheckButton = clockCheckButton;
   configDialog->clock24CheckButton = clock24CheckButton;
   configDialog->windowResComboList = windowResComboList;
   configDialog->texFilterComboList = texFilterComboList;
   configDialog->forceComboList = forceComboList;
   configDialog->filterComboList = filterComboList;
   configDialog->lodComboList = lodComboList;
   configDialog->swapComboList = swapComboList;
   return configDialog;
}

void CALL DllConfig ( HWND hParent )
{
   static ConfigDialog *configDialog = NULL;
   if (configDialog == NULL) configDialog = CreateConfigDialog();
   
   ReadSettings ();
   
   char name[21] = "DEFAULT";
   ReadSpecialSettings (name);
   
   if (gfx.HEADER)
     {
    // get the name of the ROM
    for (int i=0; i<20; i++)
      name[i] = gfx.HEADER[(32+i)^3];
    name[20] = 0;
   
    // remove all trailing spaces
    while (name[strlen(name)-1] == ' ')
      name[strlen(name)-1] = 0;
   
    ReadSpecialSettings (name);
     }
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->windowResCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->windowResComboList,
                           settings.res_data));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->fullResCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->windowResComboList,
                          settings.full_res));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->texFilterCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->texFilterComboList,
                          settings.tex_filter));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->forceCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->forceComboList,
                          settings.ucode));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->filterCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->filterComboList,
                          settings.filtering));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->lodCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->lodComboList,
                          settings.lodmode));
   
   gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(configDialog->swapCombo)->entry),
              (gchar*)g_list_nth_data(configDialog->swapComboList,
                          settings.swapmode));
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->fogCheckButton),
                settings.fog);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->bufferClearCheckButton),
                settings.buff_clear);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->autodetectCheckButton),
                settings.autodetect_ucode);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->wrapCheckButton),
                settings.wrap_big_tex);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->coronaCheckButton),
                settings.flame_corona);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->vSyncCheckButton),
                settings.vsync);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->fastcrcCheckButton),
                settings.fast_crc);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->noDitheredAlphaCheckButton),
                settings.noditheredalpha);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->noGLSLCheckButton),
                settings.noglsl);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->hiresFbCheckButton),
                settings.fb_hires);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->hiresFBOCheckButton),
                settings.FBO);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->readAllCheckButton),
                settings.fb_read_always);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->CPUWriteHackCheckButton),
                settings.cpu_write_hack);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->FBGetInfoCheckButton),
                settings.fb_get_info);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->DepthRenderCheckButton),
                settings.fb_depth_render);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->customIniCheckButton),
                settings.custom_ini);
   
   BOOL enable = !settings.custom_ini;
   gtk_widget_set_sensitive(configDialog->filterCombo, enable);
   gtk_widget_set_sensitive(configDialog->lodCombo, enable);
   gtk_widget_set_sensitive(configDialog->fogCheckButton, enable);
   gtk_widget_set_sensitive(configDialog->bufferClearCheckButton, enable);
   gtk_widget_set_sensitive(configDialog->swapCombo, enable);
   
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->FPSCheckButton),
                settings.show_fps&1);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->VICheckButton),
                settings.show_fps&2);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->ratioCheckButton),
                settings.show_fps&4);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->FPStransCheckButton),
                settings.show_fps&8);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->clockCheckButton),
                settings.clock);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialog->clock24CheckButton),
                settings.clock_24_hr);
   
   gtk_widget_show_all(configDialog->dialog);
}

