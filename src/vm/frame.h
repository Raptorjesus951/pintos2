#ifndef FRAME_H
#define FRAME_H

#include "threads/palloc.h"

struct ft_entry{
	
	void* upage;
	void* kpage;

	struct thread* t;
	bool pinned;                   // Can this frame be evicted?
	struct list_elem elem;
};

void frame_table_init(void);
void* ftalloc(enum palloc_flags flags, void* addr, uint32_t* swindx);
void ftfree(void* kpage,bool value);

#endif
