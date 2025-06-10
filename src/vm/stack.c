#include "stack.h"

bool spt_grow_stack(struct hash *spt, void *fault_addr)
{
  void *upage = pg_round_down(fault_addr);
  void *kpage = palloc_get_page(PAL_USER);
  if(kpage == NULL)
    return false;

  if(!pagedir_set_page(thread_current()->pagedir, upage, kpage, true))
  {
    palloc_free_page(kpage);
    return false;
  }

  struct spage *sp = malloc(sizeof(spage));
  sp->upage = upage;
  sp->type = PAGE_STACK;
  sp->write = true;
  spt_insert(spt, sp);
  return true;
}

