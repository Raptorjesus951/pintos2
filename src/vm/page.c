#include "page.h"
#include "swap.h"
#include "stack.h"


void spt_init(struct hash *spt)
{
  hash_init(spt, spage_hash, spage_less, NULL);
}

void spt_destroy(struct hash *spt)
{
  hash_destroy(spt, spage_destroy, NULL);
}

struct spage *spt_find(struct hash *spt, void *upage)
{
  struct spage temp;
  temp.upage = pg_round_down(upage);
  struct spage *sp = hash_find(spt, &temp.hash_elem);

  return sp != NULL ? hash_entry(sp, struct spage, hash_elem) : NULL;
}

bool spt_insert(struct hash *spt, struct spage *sp)
{
  return hash_insert(spt, &sp->hash_elem) == NULL;
}

bool spt_remove(struct hash *spt, void *upage)
{
  struct spage temp;
  temp.upage = pg_round_down(upage);
  struct hash_elem *e = hash_delete(spt, &temp.hash_elem);
  if(e != NULL)
  {
    struct spage *sp = hash_entry(e, struct spage, hash_elem);
    free(sp);
    return true;
  }

  return false;
}

bool handle_page_fault(struct hash *spt, void *fault_addr)
{
  spage *sp = spt_find(spt, fault_addr);
  if(sp == NULL)
    return false;

  void *kpage = palloc_get_page(PAL_USER);
  if(kpage == NULL)
    return false;

  switch (sp->type)
  {
    case PAGE_FILE:
      file_seek(sp->file, sp->offset);
      file_read(sp->file, kpage, sp->read_bytes);
      memset(kpage + sp->read_bytes, 0, sp->zero_bytes);
      break;
    
    case PAGE_SWAP:
      swap_in(s->swap_index, kpage);
      break;

    case PAGE_MMAP:
      file_seek(sp->file, sp->offset);
      file_read(sp->file, kpage, sp->read_bytes);
      memset(kpage + sp->read_bytes, 0, sp->zero_bytes);
      break;
  
    case PAGE_STACK:
      memset(kpage, 0, PGSIZE);
      break;

    default:
      return false;
  }

  if(!pagedir_set_page(thread_current()->pagedir, sp->upage, kpage, sp->write))
  {
    palloc_free_page(kpage);
    return false;
  }

  return true;
}
unsigned spage_hash(const struct hash_elem *e, void *aux UNUSED)
{
  const struct spage *vm_entry = hash_entry(e, vm_entry, elem);
  return hash_bytes(&vm_entry->upage, sizeof(vm_entry->page));
}

bool spage_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  const struct spage *vma = hash_entry(a, struct spage, elem);
  const struct spage *vmb = hash_entry(b, struct spage, elem);
  return vma->uaddr < vmb->uaddr;
}

void spage_destroy(struct hash_elem *elem, void *aux UNUSED)
{
  struct spage *sp = hash_entry(elem, struct spage, hash_elem);

  //TODO clean assosiated frame

  free(sp);
}
