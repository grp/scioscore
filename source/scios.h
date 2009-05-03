#ifndef SCIOS_H
#define SCIOS_H

enum
{
	SCIOS_OK = 0,
	SCIOS_TMD_OPEN = 1,
	SCIOS_TMD_READ = 2,
	SCIOS_ISFS_OPEN = 3,
	SCIOS_ISFS_READ = 4,
	SCIOS_ISFS_WRITE = 5,
	SCIOS_DIP_FIND = 6,
	SCIOS_NO_MEMORY = 7,
	SCIOS_FAIL = 8,
	SCIOS_CONTENT_MAP_LOAD = 9,
	SCIOS_CONTENT_MAP_ENTRY = 10
};

typedef struct 
{
	u32 code;
	s32 what;
	s32 data;
} sciosError;


sciosError patchIOS(u32 ios);
void debug_wait();

#endif //SCIOS_H
