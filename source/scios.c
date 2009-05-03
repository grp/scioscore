/*-------------------------------------------------------------------
SoftCorp is (c) 2009 icefire. PatchMii is (c) 2008 bushing and co. 
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <fat.h>
#include <stdarg.h>

#include "wiibasics.h"
#include "patchmii_core.h"
#include "id.h"
#include "scios.h"

#define DEBUG

void debug_sd(const char * file, u8 * buf, u32 len)
{
	#ifdef DEBUG
		FILE * map;
		map = fopen(file, "wb");
		fwrite(buf, 1, len, map);
		fclose(map);
	#endif
}

void sdprintf(const char * fmt, ...) 
{
	#ifdef DEBUG
		char buf[1024];
		s32 len;
		va_list ap;
		va_start(ap, fmt);
		len = vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
  	
		if (len <= 0 || len > sizeof(buf)) 
			printf("Error: len = %d\n", len);
		else
		{
			FILE * map;
			map = fopen("fat1:/debug_softcorp.txt", "ab");
			fwrite(buf, 1, len, map);
			fclose(map);
		}
	#endif
}


sciosError errorCreate(u32 code, s32 what, s32 data)
{
	sciosError ret;
	ret.code = code;
	ret.what = what;
	ret.data = data;
	return ret;	
}

void printErrorCode(sciosError err)
{
	switch(err.code)
	{
		case SCIOS_FAIL:
			sdprintf("Failure! Code 1: %x, Code 2: %X\n", err.what, err.data);
			break;
		case SCIOS_TMD_OPEN:
			sdprintf("TMD %d Opening Failed.\n", err.what);
			break;
		case SCIOS_TMD_READ:
			sdprintf("TMD %d Reading Failed.\n", err.what);
			break;
		case SCIOS_ISFS_OPEN:
			sdprintf("ISFS Opening Failed. File: %d, Return: %d\n", err.what, err.data);
			break;
		case SCIOS_ISFS_READ:
			sdprintf("ISFS Reading Failed. File: %d, Return: %d\n", err.what, err.data);
			break;
		case SCIOS_ISFS_WRITE:
			sdprintf("ISFS Writing Failed. File: %d, Return: %d\n", err.what, err.data);
			break;
		case SCIOS_DIP_FIND:
			sdprintf("DIP %d Finding Failed.\n", err.what);
			break;
		case SCIOS_NO_MEMORY:
			sdprintf("Memory Allocation Failed.\n");
			break;
	}
	return;	
}

void debug_wait()
{
	#ifdef DEBUG
		printf("(Debug) Press any button to continue.\n");
		wait_anyKey();
	#endif
}


sciosError patchIOS(u32 ios)
{
	char filename[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	u32 oldtmdsize ATTRIBUTE_ALIGN(32), newtmdsize ATTRIBUTE_ALIGN(32);
	s32 olddipfile ATTRIBUTE_ALIGN(32), newdipfile ATTRIBUTE_ALIGN(32), newtmdisfs ATTRIBUTE_ALIGN(32), ret ATTRIBUTE_ALIGN(32), i ATTRIBUTE_ALIGN(32);
	u64 newtitleid ATTRIBUTE_ALIGN(32), oldtitleid ATTRIBUTE_ALIGN(32), sz ATTRIBUTE_ALIGN(32);

	signed_blob * oldtmd, * newtmd;
	tmd_content * oldtmdc, * newtmdc, * oldtmdcontent = NULL, * newtmdcontent = NULL;
	u8 * data = NULL;
	
	newdipfile = newtmdisfs = sz = 0;
	
	sdprintf("(*) Installing DIP Module to IOS %d\n", ios);
	
	if(ios == 2)
		return errorCreate(SCIOS_FAIL, 0, 0);
	
	newtitleid = 0x0000000100000000LL + ios; //Add the IOS's value to the base IOS title id.
	oldtitleid = 0x00000001000000f9LL;
	
	ES_GetStoredTMDSize(oldtitleid, &oldtmdsize);
	oldtmd = (signed_blob *) memalign(32, oldtmdsize);
	if(oldtmd == NULL)
		return errorCreate(SCIOS_NO_MEMORY, 0, 0);
	memset(oldtmd, 0, oldtmdsize);
	ES_GetStoredTMD(oldtitleid, oldtmd, oldtmdsize);
	oldtmdc = TMD_CONTENTS((tmd *) SIGNATURE_PAYLOAD(oldtmd));
	
	ES_GetStoredTMDSize(newtitleid, &newtmdsize);
	newtmd = (signed_blob *) memalign(32, newtmdsize);
	if(newtmd == NULL)
		return errorCreate(SCIOS_NO_MEMORY, 0, 0);
	memset(newtmd, 0, newtmdsize);
	ES_GetStoredTMD(newtitleid, newtmd, newtmdsize);
	newtmdc = TMD_CONTENTS((tmd *) SIGNATURE_PAYLOAD(newtmd));
	
	
	sdprintf("Finding old DIP module");
	for(i = 0; i < ((tmd *) SIGNATURE_PAYLOAD(oldtmd))->num_contents; i++)
	{
		sdprintf(".");
		if(oldtmdc[i].index == 1)
		{			
			sdprintf("Found!\n");
			oldtmdcontent = &oldtmdc[i];
			sz = oldtmdcontent->size;
			
			data = (u8 *) memalign(32, sz);
				if(data == NULL)
				return errorCreate(SCIOS_NO_MEMORY, 0, 0);
			
			if(!(oldtmdcontent->type & 0x8000)) //make sure its not a shared content
			{
				sprintf(filename, "/title/00000001/000000f9/content/%08x.app", oldtmdcontent->cid); //generate filepath
					sdprintf("Read Filename: %s\n", filename);
					sdprintf("File Size: %d\n", sz);
					
				olddipfile = ISFS_Open(filename, ISFS_OPEN_RW);
					if(olddipfile < 0)
					return errorCreate(SCIOS_ISFS_OPEN, 0, olddipfile);
				ret = ISFS_Seek(olddipfile, 0, SEEK_SET);
					if(ret < 0)
					return errorCreate(SCIOS_ISFS_OPEN, 4363, olddipfile);
				ret = ISFS_Read(olddipfile, data, sz);
					if(ret < 0)
					return errorCreate(SCIOS_ISFS_READ, 0, olddipfile);
				ISFS_Close(olddipfile);
				
				//debug_sd("fat1:/data.dat", data, sz);
				FILE * thefile;
				thefile = fopen("fat1:/data.dat", "wb");
				fwrite(data, 1, sz, thefile);
				fclose(thefile);
				
				break;
			}
			else
				return errorCreate(SCIOS_FAIL, 0, 0);	
		}
		else if(i == (((tmd *) SIGNATURE_PAYLOAD(oldtmd))->num_contents) - 1) //if we made it through all of them, then we failed :(
			return errorCreate(SCIOS_DIP_FIND, 1, 0);
	}
	
	sdprintf("Finding new DIP module");
	for(i = 0; i < ((tmd *) SIGNATURE_PAYLOAD(newtmd))->num_contents; i++)
	{
		sdprintf(".");
		if(newtmdc[i].index == 1)
		{
			sdprintf("Found!\n");
			newtmdcontent = &newtmdc[i];
			
			if(newtmdcontent->type & 0x8000) //Shared content! Unshardify it
			{
				newtmdcontent->type = 0x0001;
				
				for(i = 0; i < ((tmd *) SIGNATURE_PAYLOAD(newtmd))->num_contents; i++)
				{
					if(!(newtmdc[i].type & 0x8000))
					{
						sprintf(filename, "/title/00000001/%08x/content/%08x.app", ios, newtmdc[i].cid);
						sdprintf("Found example file (filename = %s, cid = %08x, type = %04x)\n", filename, newtmdc[i].cid, newtmdc[i].type);
						break;
					}
					else if(i == (((tmd *) SIGNATURE_PAYLOAD(newtmd))->num_contents) - 1) //if we made it through all of them, then we failed :(
						return errorCreate(SCIOS_FAIL, 17, 0);
				}
						
				u8 attributes, ownerperm, groupperm, otherperm;
				u32 owner;
				u16 group;
				ret = ISFS_GetAttr(filename, &owner, &group, &attributes, &ownerperm, &groupperm, &otherperm);
					if(ret < 0)
					return errorCreate(SCIOS_FAIL, ret, 0x15F5A774); //ISFSATTR
					
				newtmdcontent->type = 0x0001;
				
				sdprintf("file attributes (owner = %d, group = %d, attributes = %d, ownerpermissions = %d, grouppermissions = %d, otherpermissions = %d)\n", owner, group, attributes, ownerperm, groupperm, otherperm);

				sprintf(filename, "/title/00000001/%08x/content/%08x.app", ios, newtmdcontent->cid); //generate filepath
				ret = ISFS_CreateFile(filename, attributes, ownerperm, groupperm, otherperm);
					if(ret < 0)
					return errorCreate(SCIOS_FAIL, ret, 0x15F5C47E); //ISFSCREATE
			}
			
			sprintf(filename, "/title/00000001/%08x/content/%08x.app", ios, newtmdcontent->cid); //generate filepath
				
			sdprintf("Write Filename: %s\n", filename);
			
			//debug_sd("fat1:/title.tmd", (u8 *) newtmd, SIGNED_TMD_SIZE(newtmd));
			//debug_sd("fat1:/dipmodule.dat", data, sz);
			
			newdipfile = ISFS_Open(filename, ISFS_OPEN_RW); //open file for writing
				if(newdipfile < 0)
				return errorCreate(SCIOS_ISFS_OPEN, 1, newdipfile);
			ret = ISFS_Seek(newdipfile, 0, SEEK_SET);
				if(newdipfile < 0)
				return errorCreate(SCIOS_ISFS_OPEN, 1, newdipfile);
			ret = ISFS_Write(newdipfile, data, sz);
				if(ret < 0)
				return errorCreate(SCIOS_ISFS_WRITE, 1, newdipfile);
			ISFS_Close(newdipfile);
			
			sprintf(filename, "/title/00000001/%08x/content/title.tmd", ios);
			newtmdisfs = ISFS_Open(filename, ISFS_OPEN_RW);
				if(newtmdisfs < 0)
				return errorCreate(SCIOS_ISFS_OPEN, 2, newtmdisfs);
			ret = ISFS_Seek(newtmdisfs, 0, SEEK_SET);
				if(newtmdisfs < 0)
				return errorCreate(SCIOS_ISFS_OPEN, 2, newtmdisfs);
			ret = ISFS_Write(newtmdisfs, newtmd, SIGNED_TMD_SIZE(newtmd));
				if(ret < 0)
				return errorCreate(SCIOS_ISFS_WRITE, 2, newtmdisfs);
			ISFS_Close(newtmdisfs);
			
			break;
		}
		else if(i == (((tmd *) SIGNATURE_PAYLOAD(newtmd))->num_contents) - 1)
			return errorCreate(SCIOS_DIP_FIND, 2, 0);
	}
	
	ISFS_Close(newdipfile);
	free(oldtmd);
	free(newtmd);
	free(data);
	
	return errorCreate(SCIOS_OK, 0, 0);
}

int main(int argc, char **argv) 
{
	s32 ret = 0;
	u32 i;
	sciosError err;
		
	//ret = IOS_ReloadIOS(5);
	ret = IOS_ReloadIOS(249);
	
	basicInit();
	WPAD_Init();
	Identify_SU();
	ISFS_Initialize();
	fatInitDefault();

	if(ret < 0)
	{
		printf("Please install IOS249 first!\n");
		
		printf("\nPress any key to quit.\n");
      	wait_anyKey();
      	return 0;
	}
	
	printf("+------------------------------------+\n");
	printf("|     SoftMii SoftCorp by icefire    |\n");
	printf("|    Completely Legal and Awesome!   |\n");
	printf("+------------------------------------+\n");

	printf("\nAny usage outside of SoftMii installation and/or personal use is prohibited without permission from icefire and the SoftMii team.\nicefire and/or the SoftMii Team hold no responsibility for any potential damage that is incurred by using this software.\n\n");
    
    printf("(*) Installing SoftCorp...\n\n");
    patchmii_network_init(); //Load PatchMii network stuff
    
    for(i = 0; i < 256; i++)
    {
    	//The next line tells whcih IOS's to install. uncomment any IOS's to isntall/patch
    	if(/*i == 9 || i == 11 || i == 12 || i == 13 || i == 14 || i == 15 || i == 17 || i == 21 || i == 21 || i == 28 || i == 30 || i == 31 || i == 33 || i == 34 || i == 35 || i == 36 ||*/ i == 37/* || i == 38 || i == 41 || i == 43 || i == 45 || i == 46 || i == 50 || i == 51 || i == 52 || i == 53 || i == 55 || i == 60 || i == 61*/)
    	{
    		printf("\n\n(*) Installing IOS %d...\n", i);

    		switch (i) //for mothballed IOS's
    		{
    			//there is no need to install ANY ios's higher than 30...
    			//but this is kept because these IOS's will fail at getting the latest version
    			/*
    			case 30:
    				//ret = patchmii_install(1, i, 1040, 1, i, 0, true);
    				break;
    			*/
    			case 50:
    				ret = patchmii_install(1, i, 4889, 1, i, 0, true);
    				break;
    			case 51:
    				ret = patchmii_install(1, i, 4633, 1, i, 0, true);
    				break;
    			case 60: //potential future mothball
    				ret = patchmii_install(1, i, 6174, 1, i, 0, true);
    				break;
    			case 61: //potential future mothball
    				ret = patchmii_install(1, i, 4890, 1, i, 0, true);
    				break;
    			default:
    			    if(i < 30)
    					ret = patchmii_install(1, 30, 1040, 1, i, 0, false); //Install 1-30 (IOS30) as 1-i and don't patch trucha (already has it)
    																		//we use IOS36 and this version so we CAN have ES_Identify()
    				else
    					ret = 1; //succeeded if we did nothing
    				break;
    		}
    		
   	 		if (ret < 0)
   	 		{
      	 		printf("(*) PatchMii error: IOS %d install failed! Support info: %d.\n", i, ret);
      	 		printf("\nPress any key to quit.\n");
      			wait_anyKey();
      			return 0;
    		}

			printf("Patching...");
    		err = patchIOS(i); //Add the DIP module
    		if(err.code != SCIOS_OK)
    		{
    			printErrorCode(err);
    			
      	 		printf("\nPress any key to quit.\n");
      	 		wait_anyKey();
      			return 0;
    		}
    		printf("..done!\n");
    	}
    }

    printf("\n\n(*) Installation Succeeded! Press any key to exit.\n\n");
    wait_anyKey();
    printf("Visit us at http://softmii.org/!");
    
    ISFS_Deinitialize();
    
	return 0;
}
