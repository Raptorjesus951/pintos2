#include "lib/kernel/bitmap.h"
#include "threads/vaddr.h"
#include "devices/block.h"
#include "vm/swap.h"

static const int SECTORS_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;
static struct bitmap* available;
static struct block* swap_block;
static int swap_size;

void bitmap_init(){
	swap_block = block_get_role(BLOCK_SWAP);
	ASSET(swap_block != NULL);
	
	swap_size = block_size(swap_block) / SECTORS_PER_PAGE;
	available = bitmap_create(swap_size);
	bitmap_set_all(available,true);
}

uint32_t swap_out(void *page){
	ASSERT(page >= PHYS_BASE);
	
	int index = bitmap_scan(available,0,1,true);
	
	int i;
	for(i = 0; i < SECTORS_PER_PAGE; i++)
		block_write(index*SECTORS_PER_PAGE +i, page + BLOCK_SECTOR_SIZE *i);
	
	bitmap_set(available,index,false);
	return index;
}
	
void swap_in (uint32_t index, void* page){
	ASSERT(page >= PHYS_BASE);

	ASSERT(index < swap_size);
	ASSERT(!bitmap_test(available,index));
	
	int i;
	for (i = 0; i < SECTORS_PER_PAGE; i++)
		block_read(index * SECTORS_PER_PAGE + i, page + BLOCK_SECTOR_SIZE * i);
	
	bitmap_set(available,index,true);
}
void swap_free(uint32_t index){
	ASSERT(index < size);
	ASSERT(!bitmap_test(available,index));
	bitmap_set(available, index, true);
}






















>>>>>>> 9c1fc2ecd19b5a3847b0e5cf9502586a39135b19
