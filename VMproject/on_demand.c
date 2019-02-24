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
	//uintptr_t assigned = petmem_alloc_pages(1);
	//printk("Buddy assigned %lx \n", assigned);

	// Map the page into page tables

	// Grab cr3
	unsigned long cr3 = get_cr3();
	printk("CR3 = %lx\n", cr3);
	//cr3 = CR3_TO_PML4E64_PA(cr3);
	//printk("CR3_TO_PML4E64_PA(CR3) = %lx\n", cr3);


	// Different experiments for values of fault_addr
	//fault_addr = CR3_TO_PML4E64_VA(cr3);
	//fault_addr = 0xffff93efffffffff;
	/*fault_addrr = kmalloc(12, GFP_KERNEL);
	char * fault_addr = (char *)fault_addrr;
	printk("---- WRITING DATA TO PAGE AT FAULT_ADDR ");
	fault_addr[0] = 'y';
	printk("(read back '%c')  ----\n", fault_addr[0]);*/
	
	// VA --> PML4E64 Index
	unsigned long pml_index = PML4E64_INDEX(fault_addr);
	printk("PML4E64 Index = %lu\n", pml_index);

	// VA --> PDPE64 Index
	unsigned long pdp_index = PDPE64_INDEX(fault_addr);
	printk("PDPE64 Index = %lu\n", pdp_index);

	// VA --> PDE64 Index
	unsigned long pde_index = PDE64_INDEX(fault_addr);
	printk("PDE64 Index =  %lu\n", pde_index);

	// VA --> PTE64 Index
	unsigned long pte_index = PTE64_INDEX(fault_addr);
	printk("PTE64 Index =  %lu\n", pte_index);
	

/*	// Walk to PML
	void * v_cr3 = __va(cr3);
	printk("(Suspected wrong) Virtual cr3 (addrs of PML table) = %lx\n", v_cr3);
	v_cr3 =  CR3_TO_PML4E64_VA(cr3);
	printk("(Suspected right) Virtual cr3 (addrs of PML table) = %lx\n", v_cr3);
	void * v_pml_dest = v_cr3 + pml_index;
	printk("PML Table + PML Index (virtual) = %lx\n", v_pml_dest);
	printk("Dereferencing...\n");
	pml4e64_t * pml_dest_data = (pml4e64_t *)v_pml_dest;
	printk("pml_dest->present = %d\n", pml_dest_data->present);
	//printk("PML entry data = %lx \n", pml_dest_data);
*/
	printk("------- Corrections ------\n");
	pml4e64_t * pml_dest;
	pdpe64_t  * pdp_dest;
	pde64_t   * pde_dest;
	pte64_t   * pte_dest;
	pml4e64_t   pml_dest_data;
	pdpe64_t    pdp_dest_data;
	pde64_t     pde_dest_data;
	pte64_t     pte_dest_data;
	unsigned long pdp_table_pg;
	unsigned long pde_table_pg;
	unsigned long pte_table_pg;
	unsigned long zeroed_user_pg;
	
	pml_dest = CR3_TO_PML4E64_VA(cr3) + pml_index*sizeof(pml4e64_t);
	//pml_dest = __va(cr3) + pml_index*sizeof(pml4e64_t);	
	printk("pml_dest = %lx\n", pml_dest);
	printk("pml_dest->present = %d\n", pml_dest->present);
   printk("(1.) pml_dest->accessed (pdp table accessed) = %d\n", pml_dest->accessed);
   pml_dest->accessed = 0;
   //invlpg(CR3_TO_PML4E64_PA(cr3)); // invl pml page 
	if(pml_dest->present == 0) {
		printk("PDP TABLE PAGE NOT PRESENT... WRITING\n");
		// Allocate page for PDP table
		pdp_table_pg = __get_free_page(GFP_KERNEL);
		printk("Received page for pdp table @ %lx\n", pdp_table_pg);
		// Create PML entry
		pml_dest_data.present = 1;
		pml_dest_data.writable = 1;
		pml_dest_data.user_page = 1;
		pml_dest_data.pdp_base_addr = PAGE_TO_BASE_ADDR(__pa(pdp_table_pg));
		// Write entry into PML table
		*pml_dest = pml_dest_data;
		printk("PML Entry: present = %d\n", pml_dest->present);
		printk("PML Entry: pdp_base_addr = %lx\n", pml_dest->pdp_base_addr);
		//invlpg(__pa(pdp_table_pg));
	}
	pdp_dest = __va(BASE_TO_PAGE_ADDR(pml_dest->pdp_base_addr)) + pdp_index*sizeof(pdpe64_t);
	printk("pdp_dest = %lx\n", pdp_dest);
	printk("pdp_dest->present = %d\n", pdp_dest->present);
	if(pdp_dest->present == 0) {		
		printk("PDE TABLE PAGE NOT PRESENT... WRITING\n");
		// Allocate page for PDE table
		pde_table_pg = __get_free_page(GFP_KERNEL);
		printk("Received page for pde table @ %lx\n", pde_table_pg);	
		// Create PDP entry
		pdp_dest_data.present = 1;
		pdp_dest_data.writable = 1;
		pdp_dest_data.user_page = 1;
		pdp_dest_data.pd_base_addr = PAGE_TO_BASE_ADDR(__pa(pde_table_pg));
		// Write entry into PDP table
		*pdp_dest = pdp_dest_data;
		printk("PDP Entry: present = %d\n", pdp_dest->present);
		printk("PDP Entry: pd_base_addr = %lx\n", pdp_dest->pd_base_addr);
	}
   //invlpg(CR3_TO_PML4E64_PA(cr3)); // invl pml page 
   printk("(2.) pml_dest->accessed (pdp table accessed) = %d\n", pml_dest->accessed);
   pml_dest->accessed = 0;
	pde_dest = __va(BASE_TO_PAGE_ADDR(pdp_dest->pd_base_addr)) + pde_index*sizeof(pde64_t);
	printk("pde_dest = %lx\n", pde_dest);
	printk("pde_dest->present = %d\n", pde_dest->present);
	if(pde_dest->present == 0) {
		printk("PTE TABLE PAGE NOT PRESENT... WRITING\n");
		// Allocate page for PTE table
		pte_table_pg = __get_free_page(GFP_KERNEL);
		printk("Received page for pte table @ %lx\n", pte_table_pg);	
		// Create PDE entry
		pde_dest_data.present = 1;
		pde_dest_data.writable = 1;
		pde_dest_data.user_page = 1;
		pde_dest_data.pt_base_addr = PAGE_TO_BASE_ADDR(__pa(pte_table_pg));
		// Write entry into PDE table
		*pde_dest = pde_dest_data;
		printk("PDE Entry: present = %d\n", pde_dest->present);
		printk("PDE Entry: pt_base_addr = %lx\n", pde_dest->pt_base_addr);
	}
	pte_dest = __va(BASE_TO_PAGE_ADDR(pde_dest->pt_base_addr)) + pte_index*sizeof(pte64_t);
	printk("pte_dest = %lx\n", pte_dest);
	printk("pte_dest->present = %d\n", pte_dest->present);
	if(pte_dest->present == 0) {
		printk("USER ACCESSED PAGE NOT PRESENT... WRITING\n");
		// Allocate ZEROED! page for user
		//zeroed_user_pg = get_zeroed_page(GFP_KERNEL);
		zeroed_user_pg = (unsigned long)petmem_alloc_pages(1);
		printk("Received BUDDY page for user @ %lx\n", zeroed_user_pg);
		// Create PTE entry
		pte_dest_data.present = 1;
		pte_dest_data.writable = 1;
		pte_dest_data.user_page = 1;
		pte_dest_data.page_base_addr = PAGE_TO_BASE_ADDR(__pa(zeroed_user_pg));
		// Write entry into PTE table
		*pte_dest = pte_dest_data;
		printk("PTE Entry: present = %d\n", pte_dest->present);
		printk("PTE Entry: page_base_addr = %lx\n", pte_dest->page_base_addr);

		// Invalidate PTE entry in case TLB cached it
		//invlpg(__pa(zeroed_user_pg));
		
	}

	printk("MMU TRACE: pml_dest->accessed = %d\n", pml_dest->accessed);
	printk("MMU TRACE: pdp_dest->accessed = %d\n", pdp_dest->accessed);
	pml_dest->accessed = 0;
	pdp_dest->accessed = 0;


	return 0;
}


