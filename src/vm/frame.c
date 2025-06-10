#include <list.h>
#include "lib/kernel/list.h"
#include <hash.h>
#include "lib/kernel/hash.h"

#include "frame.h"
#include "swap.h"
#include "page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "threads/synch.h"


static struct list frame_list;
static unsigned frame_hash_func(const struct hash_elem *elem, void *aux);
static bool     frame_less_func(const struct hash_elem *, const struct hash_elem *, void *aux);
static struct lock frame_lock;
static struct ft_entry* evicter(uint32_t pagedir);

void frame_table_init(){
	list_init(&frame_list);
	lock_init(&frame_lock);
}

void* ftalloc(enum palloc_flags flags, void* addr, uint32_t* swindx){

	lock_acquire(&frame_lock);
	void* kpage = palloc_get_page(PAL_USER|flags);
	
	if(kpage == NULL){
		struct ft_entry* f_evicted = evicter(thread_current()->pagedir);
		ASSERT(f_evicted != NULL);
		pagedir_clear_page(f_evicted->t->pagedir, f_evicted->upage);
		
		bool is_dirty = false;
		is_dirty = is_dirty || pagedir_is_dirty(f_evicted->t->pagedir, f_evicted->upage);
		is_dirty = is_dirty || pagedir_is_dirty(f_evicted->t->pagedir, f_evicted->kpage);
		
		uint32_t swap_idx = swap_out( f_evicted->kpage );
		if(swindx)
      *swindx = swap_idx;

		ftfree(f_evicted->kpage,true);
		kpage = palloc_get_page(PAL_USER|flags);
		
    PANIC("Out of memory! Kernel panicted...");
	}
	struct ft_entry* frame = malloc(sizeof(struct ft_entry));
	if (frame == NULL)
		return NULL;
	frame->kpage = kpage;
	frame->upage = addr;
	frame->t = thread_current();
	frame->pinned = true;
	list_push_back(&frame_list, &frame->elem);
	lock_release(&frame_lock);

	return kpage;
}

void ftfree(void* kpage,bool free_kpage){
	ASSERT(is_kernel_vaddr(kpage) || pg_ofs(kpage) == 0);
	
	lock_aquire(&frame_lock);

	struct ft_entry* temp;
	struct list_elem* e;
	for(e= list_begin(&frame_list); e != list_end(&frame_list); e = list_next(e)){
		temp = list_entry(e, struct ft_entry,elem);
		if (temp->kpage == kpage)
			break;
	}
	if (e == list_end(&frame_list))
		PANIC("The page can't be found");
	
	struct ft_entry* frame = list_entry(e, struct ft_entry, elem);

	list_remove(&frame->elem);

	if (free_kpage)
		palloc_free_page(kpage);
	free(frame);
	lock_release(&frame_lock);
}
struct ft_entry* evicter(uint32_t pagedir){
    static struct list_elem *clock_hand = NULL;

    lock_acquire(&frame_lock);

    if (list_empty(&frame_list)) {
        lock_release(&frame_lock);
        PANIC("No frames to evict!");
    }

    // Initialize the clock hand if it's NULL
    if (clock_hand == NULL || clock_hand == list_end(&frame_list)) {
        clock_hand = list_begin(&frame_list);
    }

    size_t num_checked = 0;
    size_t total_frames = list_size(&frame_list);

    while (num_checked < total_frames * 2) { // At most two full passes
        struct frame_entry *f = list_entry(clock_hand, struct ft_entry, elem);
        clock_hand = list_next(clock_hand);
        if (clock_hand == list_end(&frame_list)) {
            clock_hand = list_begin(&frame_list);
        }
    }

        num_checked++;

        if (f->pinned) 
	   continue;
	else if( pagedir_is_accessed(pagedir, e->upage)) {
		pagedir_set_accessed(pagedir, e->upage, false);
		continue;
	}
        bool accessed = pagedir_is_accessed(f->owner->pagedir, f->upage);
        if (accessed) {
            pagedir_set_accessed(f->owner->pagedir, f->upage, false);
        } else {
            // Found a victim
            lock_release(&frame_lock);
            return f;
        }
    }

    lock_release(&frame_table_lock);
    return NULL; // No suitable frame found (unlikely unless all are pinned)
}
static void frame_set_pinned(void* kpage, bool value){
  lock_acquire (&frame_lock);


  struct ft_entry* temp;
  struct list_elem* e;
  for(e= list_begin(&frame_list); e != list_end(&frame_list); e = list_next(e)){
  	temp = list_entry(e, struct ft_entry,elem);
  	if (temp->kpage == kpage)
  		break;
  }
  if (e == list_end(&frame_list))
  	PANIC("The page can't be found");
	
  struct ft_entry* frame = list_entry(e, struct ft_entry, elem);
  frame->pinned = value;

  lock_release (&frame_lock);
}

unsigned frame_hash_function(const struct hash_elem *e, void *aux UNUSED)
{
    const struct ft_entry *frame = hash_entry(e,struct ft_entry, elem);
    return hash_bytes(&frame->kpage, sizeof(frame->kpage));
}

bool
ft_entry_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    const struct spage *vma = hash_entry(a, struct spage, elem);
    const struct spage *vmb = hash_entry(b, struct spage, elem);
    return vma->kpage < vmb->kpage;
}
