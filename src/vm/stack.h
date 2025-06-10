#ifndef STACK_H
#define STACK_H

#include "lib/kernel/hash.h"

bool spt_grow_stack(struct hash *spt, void *fault_addr);

#endif 
