/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - savestates.c                                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Richard42 Tillin9                                  *
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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "savestates.h"
#include "main.h"
#include "translate.h"
#include "rom.h"
#include "config.h"

#include "../memory/memory.h"
#include "../memory/flashram.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"
#include "../opengl/osd.h"

const char* savestate_magic = "M64+SAVE";
const int savestate_version = 0x00010000;  /* 1.0 */

extern unsigned int interp_addr;

int savestates_job = 0;

static unsigned int slot = 0;
static int autoinc_save_slot = 0;
static char fname[1024] = {0};

void savestates_select_slot(unsigned int s)
{
    if(s<0||s>9||s==slot)
        return;
    slot = s;
    config_put_number("CurrentSaveSlot", s);

    if(rom)
        {
        char* filename = savestates_get_filename();
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Selected state file: %s"), filename);
        free(filename);
        }
    else
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Selected state slot: %d"), slot);
}

/* Returns the currently selected save slot. */
unsigned int savestates_get_slot(void)
{
    return slot;
}

/* Sets save state slot autoincrement on or off. */
void savestates_set_autoinc_slot(int b)
{
    autoinc_save_slot = b;
}

/* Returns save state slot autoincrement on or off. */
int savestates_get_autoinc_slot(void)
{
    return autoinc_save_slot != 0;
}

void savestates_inc_slot(void)
{
    if(++slot>9)
        slot = 0;
}

void savestates_select_filename(const char* fn)
{
   if(strlen((char*)fn)>=1024)
       return;
   strcpy(fname, fn);
}

char* savestates_get_filename()
{
    size_t length;
    length = strlen(ROM_SETTINGS.goodname)+4+1;
    char* filename = (char*)malloc(length);
    snprintf(filename, length, "%s.st%d", ROM_SETTINGS.goodname, slot);
    return filename;
}

void savestates_save()
{
    char *filename, *file, buffer[1024];
    unsigned char outbuf[4];
    gzFile f;
    size_t length;
    int queuelength;

    if(autoinc_save_slot)
        savestates_inc_slot();

    if(fname[0]!=0)  /* A specific filename was given. */
        {
        file = malloc(strlen(fname)+1);
        filename = malloc(strlen(fname)+1);
        strcpy(file, fname);
        strcpy(filename, fname);
        fname[0] = 0;
        }
    else
        {
        filename = savestates_get_filename();
        length = strlen(get_savespath())+strlen(filename)+1;
        file = malloc(length);
        snprintf(file, length, "%s%s", get_savespath(), filename);
        }

    f = gzopen(file, "wb");
    free(file);

    /* Write magic number. */
    gzwrite(f, savestate_magic, 8);

    /* Write savestate file version in big-endian. */
    outbuf[0] = (savestate_version >> 24) & 0xff;
    outbuf[1] = (savestate_version >> 16) & 0xff;
    outbuf[2] = (savestate_version >>  8) & 0xff;
    outbuf[3] = (savestate_version >>  0) & 0xff;
    gzwrite(f, outbuf, 4);

    gzwrite(f, ROM_SETTINGS.MD5, 32);

    gzwrite(f, &rdram_register, sizeof(RDRAM_register));
    gzwrite(f, &MI_register, sizeof(mips_register));
    gzwrite(f, &pi_register, sizeof(PI_register));
    gzwrite(f, &sp_register, sizeof(SP_register));
    gzwrite(f, &rsp_register, sizeof(RSP_register));
    gzwrite(f, &si_register, sizeof(SI_register));
    gzwrite(f, &vi_register, sizeof(VI_register));
    gzwrite(f, &ri_register, sizeof(RI_register));
    gzwrite(f, &ai_register, sizeof(AI_register));
    gzwrite(f, &dpc_register, sizeof(DPC_register));
    gzwrite(f, &dps_register, sizeof(DPS_register));
    gzwrite(f, rdram, 0x800000);
    gzwrite(f, SP_DMEM, 0x1000);
    gzwrite(f, SP_IMEM, 0x1000);
    gzwrite(f, PIF_RAM, 0x40);

    save_flashram_infos(buffer);
    gzwrite(f, buffer, 24);

    gzwrite(f, tlb_LUT_r, 0x100000*4);
    gzwrite(f, tlb_LUT_w, 0x100000*4);

    gzwrite(f, &llbit, 4);
    gzwrite(f, reg, 32*8);
    gzwrite(f, reg_cop0, 32*4);
    gzwrite(f, &lo, 8);
    gzwrite(f, &hi, 8);
    gzwrite(f, reg_cop1_fgr_64, 32*8);
    gzwrite(f, &FCR0, 4);
    gzwrite(f, &FCR31, 4);
    gzwrite(f, tlb_e, 32*sizeof(tlb));
    if(!dynacore&&interpcore)
        gzwrite(f, &interp_addr, 4);
    else
        gzwrite(f, &PC->addr, 4);

    gzwrite(f, &next_interupt, 4);
    gzwrite(f, &next_vi, 4);
    gzwrite(f, &vi_field, 4);

    queuelength = save_eventqueue_infos(buffer);
    gzwrite(f, buffer, queuelength);

    gzclose(f);
    main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Saved state to: %s"), filename);
    free(filename);
}

void savestates_load()
{
    char *filename, *file, buffer[1024];
    unsigned char inbuf[4];
    gzFile f;
    size_t length;
    int queuelength, i;

    if(fname[0]!=0)  /* A specific filename was given. */
        {
        file = malloc(strlen(fname)+1);
        filename = malloc(strlen(fname)+1);
        strcpy(file, fname);
        strcpy(filename, fname);
        fname[0] = 0;
        }
    else
        {
        filename = savestates_get_filename();
        length = strlen(get_savespath())+strlen(filename)+1;
        file = malloc(length);
        snprintf(file, length, "%s%s", get_savespath(), filename);
        }

    f = gzopen(file, "rb");
    free(file);

    if(f==NULL)
        {
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Error: state file '%s' doesn't exist"), filename);
        free(filename);
        return;
        }

    /* Read and check magic number. */
    gzread(f, buffer, 8);
    if(strncmp(buffer, savestate_magic, 8)!=0)
        {
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Error: Unrecognized savestate format."));
        free(filename);
        gzclose(f);
        return;
        }

    /* Read savestate file version in big-endian order. */
    gzread(f, inbuf, 4);
    i =            inbuf[0];
    i = (i << 8) | inbuf[1];
    i = (i << 8) | inbuf[2];
    i = (i << 8) | inbuf[3];
    if(i!=savestate_version)
        {
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Error: Savestate version (%08x) doesn't match current version (%08x)."), i, savestate_version);
        free(filename);
        gzclose(f);
        return;
        }

    gzread(f, buffer, 32);
    if(memcmp(buffer, ROM_SETTINGS.MD5, 32))
        {
        main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("Load state error: Saved state ROM doesn't match current ROM."));
        free(filename);
        gzclose(f);
        return;
        }

    gzread(f, &rdram_register, sizeof(RDRAM_register));
    gzread(f, &MI_register, sizeof(mips_register));
    gzread(f, &pi_register, sizeof(PI_register));
    gzread(f, &sp_register, sizeof(SP_register));
    gzread(f, &rsp_register, sizeof(RSP_register));
    gzread(f, &si_register, sizeof(SI_register));
    gzread(f, &vi_register, sizeof(VI_register));
    gzread(f, &ri_register, sizeof(RI_register));
    gzread(f, &ai_register, sizeof(AI_register));
    gzread(f, &dpc_register, sizeof(DPC_register));
    gzread(f, &dps_register, sizeof(DPS_register));
    gzread(f, rdram, 0x800000);
    gzread(f, SP_DMEM, 0x1000);
    gzread(f, SP_IMEM, 0x1000);
    gzread(f, PIF_RAM, 0x40);

    gzread(f, buffer, 24);
    load_flashram_infos(buffer);

    gzread(f, tlb_LUT_r, 0x100000*4);
    gzread(f, tlb_LUT_w, 0x100000*4);

    gzread(f, &llbit, 4);
    gzread(f, reg, 32*8);
    gzread(f, reg_cop0, 32*4);
    gzread(f, &lo, 8);
    gzread(f, &hi, 8);
    gzread(f, reg_cop1_fgr_64, 32*8);
    gzread(f, &FCR0, 4);
    gzread(f, &FCR31, 4);
    gzread(f, tlb_e, 32*sizeof(tlb));
    if(!dynacore&&interpcore)
        gzread(f, &interp_addr, 4);
    else
        {
        int i;
        gzread(f, &queuelength, 4);
        for (i = 0; i < 0x100000; i++)
            invalid_code[i] = 1;
        jump_to(queuelength);
        }

    gzread(f, &next_interupt, 4);
    gzread(f, &next_vi, 4);
    gzread(f, &vi_field, 4);

    queuelength = 0;
    while(1)
        {
        gzread(f, buffer+queuelength, 4);
        if(*((unsigned int*)&buffer[queuelength])==0xFFFFFFFF)
            break;
        gzread(f, buffer+queuelength+4, 4);
        queuelength += 8;
        }
    load_eventqueue_infos(buffer);

    gzclose(f);
    if(!dynacore&&interpcore)
        last_addr = interp_addr;
    else
        last_addr = PC->addr;

    main_message(0, 1, 1, OSD_BOTTOM_LEFT, tr("State loaded from: %s"), filename);
    free(filename);
}

