/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cheat.h                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Okaygo                                             *
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

#include "util.h" // list_t

#define ENTRY_BOOT 0
#define ENTRY_VI 1

extern list_t g_Cheats;

// represents all cheats for a given rom
typedef struct {
    char *rom_name;
    unsigned int crc1;
    unsigned int crc2;
    list_t cheats; // list of cheat_t's
} rom_cheats_t;

// represents a cheat for a game
typedef struct {
    char *name;
    int enabled; // cheat enabled until mupen64plus is closed
    int always_enabled; // always enabled (written to config)
    int was_enabled; // cheat was previously enabled, but has now been disabled
    list_t cheat_codes; // list of cheat_code_t's
} cheat_t;

// represents a cheatcode associated with a cheat
typedef struct {
    unsigned int address;
    unsigned short value;
    unsigned short old_value; // value before cheat was applied
} cheat_code_t;

#define CHEAT_CODE_MAGIC_VALUE 0xcafe // use this to know that old_value is uninitialized

void cheat_apply_cheats(int entry);
void cheat_read_config(void);
void cheat_write_config(void);
void cheat_delete_all(void);
void cheat_load_current_rom(void);

rom_cheats_t *cheat_new_rom(void);
cheat_t *cheat_new_cheat(rom_cheats_t *);
cheat_code_t *cheat_new_cheat_code(cheat_t *);
void cheat_delete_rom(rom_cheats_t *);
void cheat_delete_cheat(rom_cheats_t *, cheat_t *);
void cheat_delete_cheat_code(cheat_t *, cheat_code_t *);

