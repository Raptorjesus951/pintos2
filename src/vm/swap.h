void bitmap_init();
uint32_t swap_out(void *page);
void swap_in (uint32_t index, void* page);
void swap_free(uint32_t index);
