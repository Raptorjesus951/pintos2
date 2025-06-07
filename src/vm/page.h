#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>

#include "filesys/file.h"
#include "lib/kernel/hash.h"

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
  off_t offset

  size_t swap_index;

  struct hash_elem hash_elem;
};


void spt_init(struct hash *spt);
void spt_destroy(struct hash *spt);

struct spage *spt_find(struct hash *spt, void *upage);

bool spt_insert(struct hash *spt, struct spage *sp);
bool spt_remove(struct hash *spt, void *upage);


unsigned spage_hash(const struct hash_elem *e, void *aux UNUSED)
bool spage_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
void spage_destroy(struct hash_elem *elem, void *aux UNUSED);

#endif
