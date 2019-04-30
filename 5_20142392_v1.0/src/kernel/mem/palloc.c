#include <mem/palloc.h>
#include <bitmap.h>
#include <type.h>
#include <round.h>
#include <mem/mm.h>
#include <synch.h>
#include <device/console.h>
#include <mem/paging.h>
#include <proc/proc.h>

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  
   */

/* page struct */
struct kpage{
	uint32_t type;
	uint32_t *vaddr;
	uint32_t nalloc;
	pid_t pid;
};


static struct kpage *kpage_list;
static uint32_t page_alloc_index;

/* Initializes the page allocator. */
	void
init_palloc (void) 
{
	/* Calculate the space needed for the kpage list */
	size_t pool_size = sizeof(struct kpage) * PAGE_POOL_SIZE;

	/* kpage list alloc */
	kpage_list = (struct kpage *)(KERNEL_ADDR);

	/* initialize */
	memset((void*)kpage_list, 0, pool_size);
	page_alloc_index = 0;
}

/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   */
	uint32_t *
palloc_get_multiple (uint32_t page_type, size_t page_cnt)
{
	void *pages = NULL;
	struct kpage *kpage = kpage_list;
	size_t page_idx;
	int i,j;

	if (page_cnt == 0)
		return NULL;

	switch(page_type){
		case HEAP__: 
			//(1)

			/*change : find pages and alloc to that pages if kpage_list[i] is free and size equal to page_cnt*/
			for (i = 0; i < PAGE_POOL_SIZE; i++) {
				if (kpage[i].nalloc == page_cnt && kpage[i].type == FREE__) {
					for (j = i; j < i + page_cnt; j++) {
						//pages = (void*)(VKERNEL_HEAP_START - (j+1) * PAGE_SIZE);
						kpage[j].type = page_type;
						//kpage[j].vaddr = (uint32_t*)pages;
						kpage[j].nalloc = page_cnt;
						kpage[j].pid = cur_process->pid;
						pages = (void *)(kpage[j].vaddr);
					}
					break;
				}
			}

			/*change : if there is no kpage that is free*/
			if (pages == NULL) {
				for (i = 0; i < PAGE_POOL_SIZE; i++) {
					if (kpage[i].nalloc == 0) {
						j = i;
						break;
					}
				}
				for (i = j; i < j + page_cnt; i++) {
					pages = (void*)(VKERNEL_HEAP_START - (page_alloc_index + (i - j) + 1) * PAGE_SIZE);
					kpage[i].type = page_type;
					kpage[i].vaddr = (uint32_t*)pages;
					kpage[i].nalloc = page_cnt;
					kpage[i].pid = cur_process->pid;
				}
				page_alloc_index += page_cnt;
				pages = (void *)(VKERNEL_HEAP_START - page_alloc_index * PAGE_SIZE);
			}

			/*change : reset pages to 0*/
			if (pages != NULL)
			{
			    memset (pages, 0, PAGE_SIZE * page_cnt);
			}

			break;
		case STACK__: 

			/*change : find pages and alloc to that pages if kpage_list[i] is free and size equal to page_cnt*/
			for (i = 0; i < PAGE_POOL_SIZE; i++) {
				if (kpage[i].nalloc == page_cnt && kpage[i].type == FREE__) {
					for (j = i; j < i + page_cnt; j++) {
						pages = (void*)(VKERNEL_STACK_ADDR + (j - i - 2) * PAGE_SIZE);
						kpage[j].type = page_type;
						kpage[j].vaddr = (uint32_t*)pages;
						kpage[j].nalloc = page_cnt;
						kpage[j].pid = cur_process->pid;
						pages = kpage[j].vaddr;
					}
					break;
				}
			}

			/*change : if there is no kpage that is free*/
			if (pages == NULL) {
				for (i = 0; i < PAGE_POOL_SIZE; i++) {
					if (kpage[i].nalloc == 0) {
						j = i;
						break;
					}
				}
				for (i = j; i < j + page_cnt; i++) {
					pages = (void*)(VKERNEL_STACK_ADDR + (i - j - 2) * PAGE_SIZE);
					kpage[i].type = page_type;
					kpage[i].vaddr = (uint32_t*)pages;
					kpage[i].nalloc = page_cnt;
					kpage[i].pid = cur_process->pid;
				}
				//page_alloc_index += page_cnt;
			}

			pages = (void *)(VKERNEL_STACK_ADDR);

			/*change : reset pages to 0*/
			if (pages != NULL) 
				memset((void *)pages - 2 * PAGE_SIZE, 0, 2 * PAGE_SIZE);

			//(2)
			break;
		default:
			return NULL;
	}

	return (uint32_t*)pages; 
}

/* Obtains a single free page and returns its address.
   */
	uint32_t *
palloc_get_page (uint32_t page_type) 
{
	return palloc_get_multiple (page_type, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
	void
palloc_free_multiple (void *pages, size_t page_cnt) 
{
	struct kpage *kpage = kpage_list;
	int i, j;
	for (i = 0; i < page_alloc_index; i++) {
		if (((uint32_t*)pages == kpage[i].vaddr) && (kpage[i].type == HEAP__)) { //change heap to free
			for (j = i; j > i - page_cnt; j--) {
				kpage[j].type = FREE__;
				//kpage[i].vaddr = 0;
				kpage[j].nalloc = page_cnt;
				kpage[j].pid = 0;
			}
			break;
		}
	}
}

/* Frees the page at PAGE. */
	void
palloc_free_page (void *page) 
{
	palloc_free_multiple (page, 1);
}


	uint32_t *
va_to_ra (uint32_t *va){	/*change : change virtual adress to real adress*/
	int i;
	for (i = 0; i < PAGE_POOL_SIZE; i++) {
		if (kpage_list[i].vaddr == va) {
			if (kpage_list[i].type == HEAP__) {
				return (uint32_t *)(RKERNEL_HEAP_START + i * PAGE_SIZE);
			}
			else if(kpage_list[i].type == STACK__ && kpage_list[i].pid == cur_process->pid) {
				return (uint32_t *)(RKERNEL_HEAP_START + i * PAGE_SIZE);
			}
		}
	}
}

	uint32_t *
ra_to_va (uint32_t *ra){	/*change : change real adress to virtual adress*/
	int i = (((uint32_t)ra - RKERNEL_HEAP_START) / PAGE_SIZE);
	return kpage_list[i].vaddr;
}

void palloc_pf_test(void)
{
	uint32_t *one_page1 = palloc_get_page(HEAP__);
	uint32_t *one_page2 = palloc_get_page(HEAP__);
	uint32_t *two_page1 = palloc_get_multiple(HEAP__,2);
	uint32_t *three_page;
	printk("one_page1 = %x\n", one_page1); 
	printk("one_page2 = %x\n", one_page2); 
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_page(one_page1);
	palloc_free_page(one_page2);
	palloc_free_multiple(two_page1,2);

	one_page1 = palloc_get_page(HEAP__);
	two_page1 = palloc_get_multiple(HEAP__,2);
	one_page2 = palloc_get_page(HEAP__);

	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	three_page = palloc_get_multiple(HEAP__,3);

	printk("three_page = %x\n", three_page);
	palloc_free_page(one_page1);
	palloc_free_page(one_page2);
	palloc_free_multiple(two_page1,2);
	palloc_free_multiple(three_page, 3);
}

