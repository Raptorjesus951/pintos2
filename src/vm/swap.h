#ifndef SWAP_H
#define SWAP_H

void bitmap_init(void);
uint32_t swap_out(void *page);
void swap_in (uint32_t index, void* page);
void swap_free(uint32_t index);

#endif
