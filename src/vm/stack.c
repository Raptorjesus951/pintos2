#include "stack.h"
#include "frame.h"

stack_page_count = 0;

bool spt_grow_stack(struct hash *spt, void *kpage)
{
  if(stack_page_count * PAGE_SIZE >= STACK_MAX_SIZE)
    return false;

  struct spage *sp = malloc(sizeof(spage));
  if(!sp)
    return false;
  sp->upage = upage;
  sp->type = PAGE_STACK;
  sp->write = true;
 
  void *upage = pg_round_down(fault_addr);
  void *kpage = ftalloc(PAL_USER, upage, &sp->swap_index);
  if(kpage)
  {
    free(sp);
    return false;
  }
  spt_insert(spt, sp);
  return true;
}

