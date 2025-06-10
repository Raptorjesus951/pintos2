#ifndef STACK_H
#define STACK_H

#define STACK_MAX_SIZE 8000000

bool spt_grow_stack(struct hash *spt, void *kpage);

extern size_t stack_page_count;

#endif STACK_H
