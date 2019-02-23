/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>


#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"
#include "swap.h"




struct mem_map *
petmem_init_process(void)
{
	struct mem_map * this_map = kmalloc(sizeof(struct mem_map), GFP_KERNEL);
	this_map->allocated = 0;
	//INIT_LIST_HEAD(this_map->map_list);
	return this_map;
  //return NULL;
}


void
petmem_deinit_process(struct mem_map * map)
{
    
}


uintptr_t
petmem_alloc_vspace(struct mem_map * map,
		    u64              num_pages)
{
	printk("Memory allocation\n");
	if (map->allocated==1) {
		printk("mem_map already allocated! Can only pet_malloc once for now...\n");
		return 0;
	}
	map->start = PETMEM_REGION_START;
	map->size = num_pages*PAGE_SIZE_4KB;
	map->allocated = 1;
    return (uintptr_t)(map->start);
}

void
petmem_dump_vspace(struct mem_map * map)
{
    return;
}




// Only the PML needs to stay, everything else can be freed
void
petmem_free_vspace(struct mem_map * map,
		   uintptr_t        vaddr)
{
    printk("Free Memory\n");
    return;
}


/* 
   error_code:
       1 == not present
       2 == permissions error
*/

int
petmem_handle_pagefault(struct mem_map * map,
			uintptr_t        fault_addr,
			u32              error_code)
{
	printk("Page fault! At address\t %lx\n", fault_addr);
	printk("Map start:\t\t %lx\n", map->start);
	// Ask buddy for page
	uintptr_t assigned = petmem_alloc_pages(1);
	printk("Buddy assigned %lx \n", assigned);

	// Map the page into page tables

	// Grab cr3
	uintptr_t cr3 = get_cr3();
	printk("CR3 = %lx\n", cr3);
	
	cr3 = CR3_TO_PML4E64_PA(cr3);
	printk("CR3_TO_PML4E64_PA(CR3) = %lx\n", cr3);

	return -1;
}
