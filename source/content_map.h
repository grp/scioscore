#ifndef WIITOOLS_SYS_CONTENT_MAP
#define WIITOOLS_SYS_CONTENT_MAP

#include <stdint.h>

typedef struct
{
    char name[8];
    u8 sha1hash[20];
} content_map_entry;

typedef struct
{
    content_map_entry * entries;
    u32 num_entries;
} content_map;

content_map * content_map_new();
char * content_map_get_path(content_map * cm, content_map_entry * cme);
content_map_entry * content_map_get_entry_by_sha1(content_map * cm, u8 * sha1hash);
content_map_entry * content_map_get_entry_by_index(content_map * cm, u32 index);
u32 content_map_get_num_entries(content_map * cm);
void content_map_delete(content_map * cm);
void content_map_dump(content_map * cm);
#endif
