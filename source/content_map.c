#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/isfs.h>
#include <fat.h>

#include "wiibasics.h"
#include "patchmii_core.h"
#include "id.h"
#include "content_map.h"
#include "scios.h"

u32 ISFS_Size(s32 file)
{
	u32 size;
	
	fstats* stats = memalign(32, sizeof(fstats));
    ISFS_GetFileStats(file, stats);
    
    size = stats->file_length;
    free(stats);
	
	return size;
}

content_map * content_map_new()
{
    char content_map_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
    sprintf(content_map_path, "/shared1/content.map");

    s32 fd = ISFS_Open(content_map_path, ISFS_OPEN_RW);
    	if(fd < 0)
        return NULL;
    u32 size = ISFS_Size(fd);

    content_map * cm = memalign(32, sizeof(content_map));
   	 	if(cm == NULL)
    	{
        	ISFS_Close(fd);
        	return NULL;
    	}
    memset(cm, 0, sizeof(content_map));

    if(size % sizeof(content_map_entry) != 0)
    {
        ISFS_Close(fd);
        free(cm);
        return NULL;
    }

    cm->num_entries = size / sizeof(content_map_entry);
    cm->entries = memalign(32, cm->num_entries * sizeof(content_map_entry));
    
    ISFS_Read(fd, cm->entries, cm->num_entries * sizeof(content_map_entry));
    ISFS_Close(fd);

    return cm;
}

char * content_map_get_path(content_map * cm, content_map_entry * cme)
{
    if(cm==NULL || cme == NULL)
        return NULL;

    char * rv;
    asprintf(&rv, "/shared1/%.8s.app", cme->name);
    
    return rv;
}

content_map_entry * content_map_get_entry_by_sha1(content_map * cm, u8 * sha1hash)
{
	u32 i;
	
    if(cm==NULL || sha1hash==NULL)
        return NULL;
    for(i = 0; i < cm->num_entries; i++)
    {
        if(memcmp(cm->entries[i].sha1hash, sha1hash, 20) == 0)
        {
            return &cm->entries[i];
        }
    }
    return NULL;
}

content_map_entry * content_map_get_entry_by_index(content_map * cm, u32 index)
{
    if(cm==NULL)
        return NULL;

    if(index < 0 || index >=cm->num_entries)
        return NULL;

    return &cm->entries[index];
}

u32 content_map_get_num_entries(content_map * cm)
{
    if(cm==NULL)
        return 0;
    return cm->num_entries;
}

void content_map_delete(content_map * cm)
{
    if(cm)
    {
        free(cm->entries);
        cm->entries = NULL;
    }
    free(cm);
}

void content_map_dump(content_map * cm)
{
    char content_map_path[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
    
    sprintf(content_map_path, "/shared1/content.map");
    s32 fd = ISFS_Open(content_map_path, ISFS_OPEN_RW);
    	if(fd < 0)
        return;

    ISFS_Write(fd, cm->entries, cm->num_entries * sizeof(content_map_entry));
    ISFS_Close(fd);
    
    return;
}

