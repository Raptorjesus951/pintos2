#include "page.h"

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
