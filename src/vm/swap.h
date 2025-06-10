#ifndef SWAP_H
#define SWAP_H

void swap_in(size_t index, void *kpage);
void swap_out(size_t index, void *kpage);

void bitmap_init();
uint32_t swap_out(void *page);
void swap_in (uint32_t index, void* page);
void swap_free(uint32_t index);
#endif
