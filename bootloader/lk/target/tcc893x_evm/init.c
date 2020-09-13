/*
 * Copyright (C) 2011 Telechips, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <debug.h>
#include <dev/keys.h>
#include <dev/gpio_keypad.h>
#include <lib/ptable.h>
#include <dev/flash.h>
#include <platform/reg_physical.h>
#include <platform/globals.h>

#ifdef BOOTSD_INCLUDE
#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/emmc.h>
#include <partition.h>
#endif

#define LINUX_MACHTYPE  4010
	

void edi_init( void )
{
#ifdef TNFTL_V8_INCLUDE
	PEDI pEDI = (PEDI)HwEDI_BASE;

	BITCSET(pEDI->EDI_RDYCFG.nREG,  0x000FFFFF, 0x00032104 );
	BITCSET(pEDI->EDI_CSNCFG0.nREG, 0x0000FFFF, 0x00008765 );
	BITCSET(pEDI->EDI_OENCFG.nREG,  0x0000000F, 0x00000001 );
	BITCSET(pEDI->EDI_WENCFG.nREG,  0x0000000F, 0x00000001 );	
#endif
}

void target_init(void)
{
	dprintf(ALWAYS, "target_init()\n");

#if !defined(TARGET_BOARD_DAUDIO)
	keys_init();
	keypad_init();
#endif

#if defined(BOOTSD_INCLUDE)
	if (target_is_emmc_boot())
	{
		emmc_printpartition();
#if 0
		emmc_boot_main();
#endif
		return ;
	}
#endif

#if defined(NAND_BOOT_INCLUDE)
	edi_init();
	if (check_fwdn_mode()) {
		fwdn_start();
		return;
	}
	flash_boot_main();
#endif
}

unsigned board_machtype(void)
{
	return LINUX_MACHTYPE;
}

void reboot_device(unsigned reboot_reason)
{
	reboot(reboot_reason);
}

int board_get_serialno(char *serialno)
{
	int n,i;
	char temp[32];
#ifdef BOOTSD_INCLUDE
	if (target_is_emmc_boot())
		n = get_emmc_serial(temp);
	else
#endif
#if defined(NAND_BOOT_INCLUDE)
		n = NAND_GetSerialNumber(temp, 32);
#else
		return 0;
#endif
	for (i=0; i<4; i++)	// 4 = custon field(2) + product number(2)
		*serialno++ = temp[i];
	for (i=16; i<32; i++)	// 16 = time(12) + serial count(4)
		*serialno++ = temp[i];
	*serialno = '\0';
	return strlen(serialno);
}

int board_get_wifimac(char *wifimac)
{
	int n,i;
	char temp[32];
#ifdef BOOTSD_INCLUDE
	if (target_is_emmc_boot())
		n = get_emmc_serial(temp);
	else
#endif
#if defined(NAND_BOOT_INCLUDE)
		n = NAND_GetSerialNumber(temp, 32);
#else
		return 0;
#endif
	if (temp[1] == '1') {
		for (i=0; i<12; i++) {
			*wifimac++ = temp[4+i];
			if (i==11) break;
			if (!((i+1)%2)) *wifimac++ = ':';
		}
	} else if(temp[1] == '2') {
		for (i=0; i<12; i++) {
			*wifimac++ = temp[16+i];
			if (i==11) break;
			if (!((i+1)%2)) *wifimac++ = ':';
		}
	}
	*wifimac = '\0';
	return strlen(wifimac);
}

int board_get_btaddr(char *btaddr)
{
	int n,i;
	char temp[32];
#ifdef BOOTSD_INCLUDE
	if (target_is_emmc_boot())
		n = get_emmc_serial(temp);
	else
#endif
#if defined(NAND_BOOT_INCLUDE)
		n = NAND_GetSerialNumber(temp, 32);
#else
		return 0;
#endif
	for (i=4; i<16; i++)	// 12 = bluetooth bd address field(12)
		*btaddr++ = temp[i];
	*btaddr = '\0';
	return strlen(btaddr);
}
