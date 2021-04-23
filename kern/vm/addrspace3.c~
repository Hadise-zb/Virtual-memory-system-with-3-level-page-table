#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <elf.h>
#include <synch.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */
 /*
vaddr_t alloc_frame(struct addrspace *as, vaddr_t vaddr) {
    (void)as;
    vaddr = vaddr;
	
    vaddr_t newVaddr = alloc_kpages(1);
	if (newVaddr == 0) return 0;
	
    bzero((void *) newVaddr, PAGE_SIZE);
	return newVaddr;
} */

int TLB_f(int spl)
{
    int i = 0;
    while(i < NUM_TLB) {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        i++;
    }
    return spl;
}

int PTE_copy(struct addrspace *old, struct addrspace *newas) {
    //newas -> pagetable = (paddr_t ***)kmalloc(sizeof(paddr_t **) * 256);
    //if (newas -> pagetable == NULL) return 1;
    for (int i = 0; i < 256; i++) {
        if (old->pagetable[i] == NULL) {
            newas->pagetable[i] = NULL;
            continue;
        }
        newas -> pagetable[i] = kmalloc(4 * 64);
        if (newas -> pagetable[i] == NULL) return 1;
        for (int j = 0; j < 64; j++) {
            if (old->pagetable[i][j] != NULL) {
			    newas -> pagetable[i][j] = kmalloc(4 * 64);
                if (newas -> pagetable[i][j] == NULL) return 1;
                for (int k = 0; k < 64; k++){
                    if (old -> pagetable[i][j][k] != 0) {
			            //kprintf("alloc_frame_before\n");
			            vaddr_t new_frame_addr = alloc_frame(old, PADDR_TO_KVADDR(old->pagetable[i][j][k] & PAGE_FRAME));
			            //kprintf("alloc_frame_after\n");
					    //if (new_frame_addr == 0) {
					    //    kprintf("HaHa\n");
					    //}
					
					    memmove((void *)new_frame_addr, (void *)PADDR_TO_KVADDR(old->pagetable[i][j][k] & PAGE_FRAME), PAGE_SIZE);
					
					    newas->pagetable[i][j][k] = KVADDR_TO_PADDR(new_frame_addr) & PAGE_FRAME;
					    newas->pagetable[i][j][k] = newas->pagetable[i][j][k] | (TLBLO_DIRTY & old->pagetable[i][j][k]) | (TLBLO_VALID & old->pagetable[i][j][k]);
			        } else {
			            newas -> pagetable[i][j][k] = 0;
			        }
                }
            } else {
                newas->pagetable[i][j] = NULL;
            }
	    }
	}    
    return 0;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */

    as->regions = NULL;
    //kprintf("as_create_kmalloc_before\n");
	if ((as->pagetable = kmalloc(4 * 256)) == NULL)
	{
	    kfree(as);
		return NULL;
	}
    //kprintf("as_create_kmalloc_after\n");
    for (int i = 0; i < 256; i++) as -> pagetable[i] = NULL;
    
    as->pt_lock = lock_create("create a new pt_lock");
    
	return as;
}



int
as_copy(struct addrspace *old, struct addrspace **ret)
{
    //allocates a new destination addr space
    if (old == NULL) {
		return EINVAL;
	}
	
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
    
    lock_acquire(old->pt_lock);
    
    //adds all the same regions as source
    struct region *old_regions = old->regions;
    //struct region *temp = NULL;
    
    struct region *new_curr = newas->regions;
    
    while(old_regions != NULL) {
        struct region *new;
        if ((new = kmalloc(sizeof(struct region))) == NULL) {
            //kfree(new);
            as_destroy(newas);
            lock_release(old->pt_lock);
            return ENOMEM;
        }
        new->base_addr = old_regions->base_addr;
		new->memsize = old_regions->memsize;
		new->writeable = old_regions->writeable;
		new->old_writeable = old_regions->old_writeable;
		new->readable = old_regions->readable;
		new->executable = old_regions->executable;
		//new->next = newas->regions;
		new->next = NULL;
		
		if(new_curr != NULL) {
			new_curr->next = new;
		} else {
			newas->regions = new;
		}
		new_curr = new;
		/*
		if (temp == NULL) {
		    temp = new; 
		} else {
		    //newas->regions = new;
		    temp->next = new;
		    temp = new;
		}*/
		
		old_regions = old_regions->next;
    }
    
    int result = PTE_copy(old, newas);
	if ((result != 0)) {
		as_destroy(newas);
		lock_release(old->pt_lock);
		return ENOMEM;
	}
    
    lock_release(old->pt_lock);
	//(void)old;

	*ret = newas;
	return 0;
}




void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	if(as == NULL) return;

    lock_acquire(as->pt_lock);

    //Free page table
    int i = 0;
    while (i < 256) {
        if (as->pagetable[i] != NULL) {
            int j = 0;
            while (j < 64) {
                if (as->pagetable[i][j] != NULL) {
                    int k = 0;
                    while (k < 64) {
                        if (as-> pagetable[i][j][k] != 0) {
                                
                            free_kpages(PADDR_TO_KVADDR(as->pagetable[i][j][k]) & PAGE_FRAME);
                             
                        } 
                        k++; 
                    }     
                }
                kfree(as->pagetable[i][j]);
                j++;
            }
            
        }
        kfree(as->pagetable[i]);
        i++;
    }
	
	kfree(as->pagetable);
	
	// Free vitual address
	struct region *curr = as->regions;
	struct region *temp;
	while (curr != NULL)
	{
		
	    temp = curr->next;
		kfree(curr);
		curr = temp;
		/*
		temp = curr;
		curr = curr->next;
		kfree(temp);	*/
	}
	lock_release(as->pt_lock);
	lock_destroy(as->pt_lock);
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/*
	 * Write this.
	 */
	int s = splhigh();
	
	int spl = TLB_f(s);

	splx(spl);
}


void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
	 as_activate();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE writeable are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
   int readable, int writeable, int executable)
{
    if (as == NULL) return EFAULT;

    // 
    memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
    vaddr &= PAGE_FRAME;
    memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;
    //kprintf("debug1\n");
    // 
    int result = region_valid(as, vaddr, memsize);
	if (result) {
		return result; 
	}
	
	struct region *new = region_create(vaddr, memsize, readable, writeable, executable);
	if (new == NULL) return ENOMEM;
	region_insert(as, new);
	return 0;
	
    
    
    

}
void region_insert(struct addrspace *as, struct region *new) {
	new->next = as->regions;
	as->regions = new;
}

struct region * region_create(vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable) {

	struct region *new = kmalloc(sizeof(struct region));
	if (new != NULL) {
		new->base_addr = vaddr;
		new->memsize = memsize;
		new->readable = readable;
		new->writeable = writeable;
		new->executable = executable;
		new->next = NULL;
	}

	return new;
}
int region_valid(struct addrspace *as, vaddr_t vaddr, size_t memsize) {
	if ((vaddr + memsize) > MIPS_KSEG0) {
		return EFAULT;
	}

	
	struct region *curr = as->regions;
	while(curr != NULL) {
		if (curr->base_addr <= vaddr && 
			(curr->base_addr + curr->memsize) > vaddr) {
			return EINVAL; //TODO Check correct error
		} else if (curr->base_addr <= (vaddr+memsize) && 
			(curr->base_addr + curr->memsize) > (vaddr+memsize)) {
			return EINVAL;
		} else if (vaddr <= curr->base_addr && 
			(vaddr + memsize) > curr->base_addr) {
			return EINVAL;
		} else if (vaddr <= (curr->base_addr + curr->memsize) && 
			(vaddr + memsize) > (curr->base_addr + curr->memsize)) {
			return EINVAL;
		}
		curr = curr->next;
	}
	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	if (as == NULL) return EFAULT; // Bad memory reference

	struct region *oRegions = as -> regions;
	while (oRegions != NULL) {
		// make READONLY regions to RW
		oRegions->old_writeable = oRegions->writeable;
		oRegions->writeable = 1;
		oRegions = oRegions -> next;
	}
	// panic("addrspace: as_prepare_load DONE\n");
	
	
	//(void)as;
	return 0;
}

struct region *get_region2(struct addrspace *as, vaddr_t vaddr) {
    struct region *curr = as->regions;
    while (curr != NULL) {
        if (vaddr >= curr->base_addr &&
            (vaddr < (curr->base_addr + curr->memsize))) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

//restore saved permissions
int
as_complete_load(struct addrspace *as)
{
	
    if (as == NULL) return EFAULT; // Bad memory reference
    
    as_activate(); //Flush after loading
	lock_acquire(as->pt_lock);
	paddr_t ***pt = as->pagetable;
	for(int i = 0; i < 256; i++) {
		if (pt[i] != NULL) {
			for(int j = 0; j < 64; j++) {
			    if (pt[i][j] != NULL) {
			        for (int k = 0; k < 64; k++) {
			            if (pt[i][j][k] != 0) {
					        vaddr_t pd_bits = i << 24;
					        vaddr_t pt_bits = j << 18;
					        vaddr_t pk_bits = k << 12;
					        vaddr_t vaddr = pd_bits|pt_bits|pk_bits;
					        struct region *region = get_region2(as, vaddr);
					        
					        if (region != NULL) {
					            if (region->old_writeable == 0) {
					                //kprintf("third_page_check");
						            pt[i][j][k] = (pt[i][j][k] & PAGE_FRAME) |  TLBLO_VALID;
					            }
					            //kprintf("debug2\n");
					        } else {
					            //kprintf("debug3\n");
					        }
				        }
			        }
			    }
				
			}
		}
	}
	lock_release(as->pt_lock);
	struct region *curr = as->regions;
	while(curr != NULL) {
		curr->writeable = curr->old_writeable;
		curr = curr->next;
	}
    
	//(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	//(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	int output;
	
	if ((output = as_define_region(as, *stackptr - 16 * PAGE_SIZE, 16 * PAGE_SIZE, 1, 1, 0)) != 0) {
	    return output;
	}

	return 0;
}

