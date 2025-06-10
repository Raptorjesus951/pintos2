#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>

#include "filesys/file.h"
#include "lib/kernel/hash.h"

#define PAGE_SIZE 4096

enum page_type
{
  PAGE_FILE,
  PAGE_SWAP,
  PAGE_STACK,
  PAGE_MMAP
};

struct spage
{
  void *upage; //user page
  void *kpage; //kernek page

  enum page_type type;
  
  bool write;
  bool loaded;

  //for file backed pages
  struct file *file;
  off_t offset;

  size_t read_bytes;
  size_t zero_bytes;
  
  uint32_t swap_index;

  struct hash_elem hash_elem;
};


void spt_init(struct hash *spt);
void spt_destroy(struct hash *spt);

struct spage *spt_find(struct hash *spt, void *upage);

bool spt_insert(struct hash *spt, struct spage *sp);
bool spt_remove(struct hash *spt, void *upage);

bool handle_page_fault(struct hash *spt, void *fault_addr);

bool spt_grow_stack(struct hash *spt, void *fault_addr);

unsigned spage_hash(const struct hash_elem *e, void *aux (UNUSED));
bool spage_less(const struct hash_elem *a, const struct hash_elem *b, void *aux (UNUSED));
void spage_destroy(struct hash_elem *elem, void *aux (UNUSED));

#endif
