/*  patches.c -- Various patches for use with patchmii

    Copyright (C) 2008 tona

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gccore.h>

#include "patchmii_core.h"
#include "patches.h"

int patch_hash_check(u8 *buf, u32 size)
 {
  u32 i;
  u32 match_count = 0;
  u8 new_hash_check[] = {0x20,0x07,0x4B,0x0B};
  u8 old_hash_check[] = {0x20,0x07,0x23,0xA2};
  
  for (i=0; i<size-4; i++) {
    if (!memcmp(buf + i, new_hash_check, sizeof(new_hash_check))) {
      debug_printf("Found new-school ES hash check @ 0x%x, patching.\n", i);
      buf[i+1] = 0;
      i += 4;
      match_count++;
      continue;
    }

    if (!memcmp(buf + i, old_hash_check, sizeof(old_hash_check))) {
      debug_printf("Found old-school ES hash check @ 0x%x, patching.\n", i);
      buf[i+1] = 0;
      i += 4;
      match_count++;
      continue;
    }
  }
  return match_count;
}

