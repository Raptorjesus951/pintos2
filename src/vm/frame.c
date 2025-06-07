#include <list.h>
#include "lib/kernel/list.h"
#include <hash.h>
#include "lib/kernel/hash.h"

#include "frame.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

static struct hash frame_hash;
static unsigned frame_hash_func(const struct hash_elem *elem, void *aux);
static bool     frame_less_func(const struct hash_elem *, const struct hash_elem *, void *aux);

struct ft_entry{
	
	void* upage;
	void* kpage;

	struct hash_elem elem;
	struct thread* t;
};

void frame_table_init(){
	hash_init(&frame_hash,frame_hash_func,ft_entry_less,NULL);
}

void* ftalloc(enum palloc_flags flags, void* addr){

	void* kpage = palloc_get_page(PAL_USER|flags);
	
	ASSERT(page != NULL) //TODO change to condiction for eviction
	
	struct ft_entry* frame = malloc(sizeof(struct ft_entry));
	if (frame == NULL)
		return NULL;
	frame->kpage = kpage;
	frame->upage = addr;
	frame->t = thread_current();
	hash_insert(&frame_list, &frame->elem);

	return kpage;
}

void ftfree(void* kpage,bool free_kpage){
	ASSERT(is_kernel_vaddr(kpage) || pg_ofs(kpage) == 0);
	
	struct ft_entry* temp;
	temp.kpage = kpage;

	struct hash_elem* h= hash_find(&fram_hash, &(temp.kpage));
	if (h == NULL)
		PANIC("The page can't be found");
	
	struct ft_entry* frame = hash_entry(h, struct ft_entry, helem);

	hash_delete(&frame_hash,&frame->elem);

	if (free_kpage)
		palloc_free_page(kpage);
	free(frame);
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
