#include "stack.h"
#include "frame.h"
#define STACK_MAX_SIZE 8000000

stack_page_count = 0;

bool spt_grow_stack(struct hash *spt, void *fault_addr)
{
  if (stack_page_count * PAGE_SIZE >= STACK_MAX_SIZE)
    return false;

  void *upage = pg_round_down(fault_addr);
  void *kpage = ftalloc(PAL_USER, upage, NULL);
  if (kpage == NULL)
    return false;

  struct spage *sp = malloc(sizeof(struct spage));
  if (!sp)
    return false;

  sp->upage = upage;
  sp->kpage = kpage;
  sp->type = PAGE_STACK;
  sp->write = true;
  sp->loaded = true;

  if (!spt_insert(spt, sp))
  {
    free(sp);
    return false;
  }

  stack_page_count++;
  return true;
}


