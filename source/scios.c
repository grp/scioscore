/*-------------------------------------------------------------------
scioscore is (c) 2009 icefire.

PatchMii is (c) 2008 bushing et al.
Identify code by tona, modified by icefire.
+-------------------------------------------------------------------*/
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

#include "patchmii_core.h"
#include "scios.h"
#include "sha1.h"
#include "certs_dat.h"

#define DEBUG

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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
			sprintf(buf, "%s\n", buf);
			FILE * map = fopen("sd:/scioscore.log", "ab");
			fprintf(map, "%s", buf);
			fclose(map);
			printf("%s", buf);
		}
	#endif
}


void wait()
{
	#ifdef DEBUG
		printf("(Debug) Press any button to continue.\n");
		WPAD_ScanPads();
		while(WPAD_ButtonsDown(0) == 0)
			WPAD_ScanPads();
	#endif
}

s32 getDIP()
{
	char filename[ISFS_MAXPATH];
	u32 tmdsize, sz;
	s32 ret, i, fd;

	signed_blob * stmd;
	tmd_content * tmdc;
	u8 * data = NULL;
	
	ES_GetStoredTMDSize(0x00000001000000f9LL, &tmdsize);
	stmd = (signed_blob *) memalign(32, tmdsize);
	//if(stmd == NULL)
	//	return -1;
	memset(stmd, 0, tmdsize);
	ES_GetStoredTMD(0x00000001000000f9LL, stmd, tmdsize);
	tmdc = TMD_CONTENTS((tmd *) SIGNATURE_PAYLOAD(stmd));
	
	for(i = 0; i < ((tmd *) SIGNATURE_PAYLOAD(stmd))->num_contents; i++)
	{
		if(tmdc[i].index == 1) //dip module is index 1, always
		{			
			sz = tmdc[i].size;
			
			data = (u8 *) memalign(32, sz);
			//if(data == NULL)
			//	return -1;
			
			if(!(tmdc[i].type & 0x8000)) //make sure its not a shared content
			{
				sprintf(filename, "/title/00000001/000000f9/content/%08x.app", tmdc[i].cid); //generate filepath
				sdprintf("DIP filename: %s", filename);
				
				fd = ISFS_Open(filename, ISFS_OPEN_RW);
				//if(fd < 0)
				//	return -1;
				ret = ISFS_Seek(fd, 0, SEEK_SET);
				//if(ret < 0)
				//	return -1;
				ret = ISFS_Read(fd, data, sz);
				//if(ret < 0)
				//	return -1;
				ISFS_Close(fd);
				
				FILE * fat = fopen("sd:/dip.bin", "wb");
				fwrite(data, 1, sz, fat);
				fclose(fat);
				
				sdprintf("DIP Module extracted");
				
				return 0;
			}
			else
				return -1;
		}
		else if(i == (((tmd *) SIGNATURE_PAYLOAD(stmd))->num_contents) - 1) //if we made it through all of them, then we failed :(
			return -1;
	}
	return -1; //we only get here if we fail.
}


s32 install_cIOSCORP()
{
	static u64 * titles;
	s32 ret, i, j, cnt;
	u32 num_titles, ios_cnt = 0;
	u32 * ios;
	
	getDIP();
	
	ret = ES_GetNumTitles(&num_titles);
	//if(ret < 0)
	//	return -1;
	titles = memalign(32, num_titles * sizeof(u64));
	ret = ES_GetTitles(titles, num_titles);
	//if(ret < 0)
	//	return ret;
		
	sdprintf("Number of titles intalled is: %u", num_titles);
	
	
	for (i = 0; i < num_titles; i++) 
	{
		u32 upper;
		upper = titles[i] >> 32;
		if(upper == 0x00000001)
			ios_cnt++;
	}	
	ios = memalign(32, ios_cnt * sizeof(u32));
	
	for (i = 0, j = 0; j < ios_cnt && i < num_titles; i++)
	{
		u32 upper, lower;
		upper = titles[i] >> 32;
		lower = titles[i] & 0xFFFFFFFF;
		if(upper == 0x00000001)
		{
			ios[j] = lower;
			j++;
		}
	}
	free(titles);
	
	sdprintf("Installed IOS count: %u", ios_cnt);
	
	sdprintf("Loading certs from NAND...");
	s32 ifd = ISFS_Open("/sys/cert.sys", ISFS_OPEN_READ);
	u32 cert_len = 0x0A00;
	signed_blob * certs = memalign(32, cert_len);
	ISFS_Read(ifd, certs, cert_len);
	ISFS_Close(ifd);
	sdprintf("Loaded certificate chain!");
	
	for(cnt = 0; cnt < ios_cnt; cnt++) //patch all the ios's
	{
		u32 cur_ios = ios[cnt];
		if(cur_ios == 249 || cur_ios == 254 || cur_ios == 4 || cur_ios == 3 || cur_ios == 0)
			continue;
			
		if(cur_ios != 28)
		{
			sdprintf("Skipping IOS %u...", cur_ios);
			continue;
		}
			
		if(cur_ios < 30)
			patchmii_install(1, 30, 1040, 1, cur_ios, 0, false);
		u32 tmdsize;
		u64 titleid;
		
		sdprintf("\nCurrent IOS: %u", cur_ios);
		
		
		signed_blob * stmd;
		tmd_content * tmdc;
		tmd * ptmd;

		titleid = 0x0000000100000000ULL | cur_ios; //make a full title id
	
		ES_GetStoredTMDSize(titleid, &tmdsize);
		stmd = (signed_blob *) memalign(32, tmdsize);
		//if(stmd == NULL)
		//	return -1;
		memset(stmd, 0, tmdsize);
		ES_GetStoredTMD(titleid, stmd, tmdsize);
		ptmd = SIGNATURE_PAYLOAD(stmd);
		tmdc = TMD_CONTENTS(ptmd);
		
		sdprintf("TMD Retrieved! Size: %u, number of contents: %u", tmdsize, ptmd->num_contents);
	
		u8 * buf = NULL;
		u32 fsz = 0;
		for(i = 0; i < ptmd->num_contents; i++)
		{
			char * fname = malloc(256);
			sprintf(fname, "sd:/%08x", i);
			sdprintf("SD Filename for content index %u out of %u: %s", i, ptmd->num_contents, fname);
			
			if(tmdc[i].index == 1) //dip module
			{
				sdprintf("DIP module copying...");
				FILE * src = fopen("sd:/dip.bin", "rb");
				fseek(src, 0, SEEK_END);
				fsz = ftell(src);
				fseek(src, 0, SEEK_SET);
				sdprintf("filesize: %u", fsz);
				buf = malloc(fsz);
				fread(buf, 1, fsz, src);
				fclose(src);
				
				FILE * dst = fopen(fname, "wb");
				fwrite(buf, 1, fsz, dst);
				fclose(dst);
				
				sdprintf("DIP module copied");
				
				sha1 hash;
				SHA1(buf, fsz, hash);
				memcpy(tmdc[i].hash, hash, 20);
				free(buf);
				sdprintf("Updated hash!");
				
				tmdc[i].size = fsz;
			}
			else
			{
				sdprintf("Reading content from NAND...");
				
				s32 cfd;
				u8 * data = memalign(32, tmdc[i].size);
				cfd = ES_OpenTitleContent(titleid, i);
				ES_ReadContent(cfd, data, tmdc[i].size);
				ES_CloseContent(cfd);

				FILE * dst = fopen(fname, "wb");
				fwrite(data, tmdc[i].size, 1, dst);
				fclose(dst);
				sdprintf("Done writing content to SD.", i);
				
				sha1 hash;
				SHA1(data, tmdc[i].size, hash);
				memcpy(tmdc[i].hash, hash, 20);
				free(data);
				sdprintf("Updated hash!");
			}
			
			tmdc[i].cid = i; //REALLY FUCKING HACKY. WHY THE HELL ISNT THE CID THERE ANYWAY!? -- not working anyway
							
			sdprintf("Content read and written to SD (size: %u bytes, content id: %u)", tmdc[i].size, tmdc[i].cid);
			free(fname);
		}
		
		u16 i; //fakesign!
		for(i = 0; i < 65535; i++) 
		{
			ptmd->fill3 = i;
			sha1 hash;
			SHA1((u8 *) ptmd, TMD_SIZE(ptmd), hash);
			if(hash[0] == 0) 
			{
				printf("Fakesigned TMD!");
				break;
			}
		}
		

		ret = ES_AddTitleStart(stmd, tmdsize, certs, cert_len, NULL, 0);
		sdprintf("AddTitle returned %d", ret);
		for(i = 0; i < ptmd->num_contents; i++)
		{
			sdprintf("Adding content: %u", i);
			char * fname = malloc(256);
			sprintf(fname, "sd:/%08x", i);
			FILE * fp = fopen(fname, "rb");
			free(fname);
			u8 * cur_data;
			s32 cfd;
			
			cur_data = memalign(32, tmdc[i].size);
			fread(cur_data, 1, tmdc[i].size, fp);
		
			cfd = ES_AddContentStart(titleid, tmdc[i].cid);
			sdprintf("cfd/return value from ES_AddContentStart() is %d", cfd);
			ret = ES_AddContentData(cfd, cur_data, tmdc[i].size);
			sdprintf("ES_AddContentData() returned %d", ret);
			ES_AddContentFinish(cfd);
			
			sdprintf("Done adding content %d of %d", i, ptmd->num_contents);
		
			free(cur_data);
			fclose(fp);
			unlink(fname);
		}
		ES_AddTitleFinish();
		free(stmd);
	}
	free(ios);
	free(certs);
	
	return 0;
}

int main(int argc, char **argv) 
{
	s32 ret = IOS_ReloadIOS(249); //load cios
	
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	GX_AdjustForOverscan(rmode, rmode, 32, 24);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	
	printf("\x1b[2;0H");

	WPAD_Init();
	ISFS_Initialize();
	fatInitDefault();

	if(ret < 0)
	{
		sdprintf("Please install IOS249 first!\n");
		
		sdprintf("\nPress any key to quit.\n");
      	WPAD_ScanPads();
		while(WPAD_ButtonsDown(0) == 0)
			WPAD_ScanPads();
      	return 0;
	}
	
	sdprintf("+------------------------------------+\n");
	sdprintf("|    scioscore by icefire @ WADder   |\n");
	sdprintf("|    Completely Legal and Awesome!   |\n");
	sdprintf("+------------------------------------+\n");

    sdprintf("(*) Installing...\n");
	patchmii_network_init(); //Load PatchMii network stuff
   
    install_cIOSCORP(); 
   
	sdprintf("\n\n(*) Installation Succeeded! Press any key to exit.\n\n");
    
	while(WPAD_ButtonsDown(0) == 0)
		WPAD_ScanPads();
    
    ISFS_Deinitialize();
    
	return 0;
}
