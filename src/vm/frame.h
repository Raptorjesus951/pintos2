#ifndef FRAME_H
#define FRAME_H

void frame_table_init();
void* ftalloc(enum palloc_flags flags, void* addr, uint32_t* swindx);
void ftfree(void* kpage);

#endif
