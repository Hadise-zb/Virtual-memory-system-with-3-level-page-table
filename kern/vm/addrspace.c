/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
vaddr_t alloc_frame(struct addrspace *as, vaddr_t vaddr) {
	/*  Allocate Frame for this region  */
    (void)as;
    vaddr = vaddr;
	
    vaddr_t newVaddr = alloc_kpages(1);
	if (newVaddr == 0) return 0;
	
    bzero((void *) newVaddr, PAGE_SIZE);
	return newVaddr;
} 

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
    newas -> pagetable = (paddr_t ***)kmalloc(sizeof(paddr_t **) * 256);
    if (newas -> pagetable == NULL) return 1;
    for (int i = 0; i < 256; i++) {
        if (old->pagetable[i] == NULL) {
            newas->pagetable[i] = NULL;
            continue;
        }
        newas -> pagetable[i] = (paddr_t **)kmalloc(sizeof(paddr_t *) * 64);
        if (newas -> pagetable[i] == NULL) return 1;
        for (int j = 0; j < 64; j++) {
			newas -> pagetable[i][j] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
            if (newas -> pagetable[i][j] == NULL) return 1;
            for (int k = 0; k < 64; k++){
                if (newas -> pagetable[i][j][k] != 0) {
			        //vaddr_t new_frame_addr = alloc_kpages(1);
			        vaddr_t new_frame_addr = alloc_frame(old, PADDR_TO_KVADDR(old->pagetable[i][j][k]));
					//newas->pagetable[i][j][k] = KVADDR_TO_PADDR(new_frame_addr) & 0xfffff000;
					newas->pagetable[i][j][k] = KVADDR_TO_PADDR(new_frame_addr) & PAGE_FRAME;
					memmove((void *)new_frame_addr, (const void *)PADDR_TO_KVADDR(old->pagetable[i][j][k] & PAGE_FRAME), 64);
			    } else {
			        newas -> pagetable[i][j][k] = 0;
			    }
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

	if ((as->pagetable = kmalloc(sizeof(paddr_t **) * 256)) == NULL)
	{
		return NULL;
	}

    for (int i = 0; i < 256; i++) as -> pagetable[i] = NULL;
    
	return as;
}



int
as_copy(struct addrspace *old, struct addrspace **ret)
{
    //allocates a new destination addr space
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
    
    //adds all the same regions as source
    struct region *old_regions = old->regions;
    struct region *temp = NULL;
    while(old_regions != NULL) {
        struct region *new;
        if ((new = kmalloc(sizeof(struct region))) == NULL) {
            //kfree(new);
            return ENOMEM;
        }
        new->region_base = old_regions->region_base;
		new->npages = old_regions->npages;
		new->flags = old_regions->flags;
		new->prev_flags = old_regions->prev_flags;
		//new->next = newas->regions;
		new->next = NULL;
		if (temp == NULL) {
		    temp = new; 
		} else {
		    //newas->regions = new;
		    temp->next = new;
		    temp = new;
		}
				
		old_regions = old_regions->next;
    }
    //roughly, for each mapped page in source
        
    //allocate a frame in dest
    
    //copy contents from source frame to dest frame
    
    //add PT entry for dest
    int result = PTE_copy(old, newas);
	if ((result != 0)) {
		as_destroy(newas);
		return ENOMEM;
	}
    

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

    //Free page table
    int i = 0;
    while (i < 256) {
        if (as->pagetable[i] != NULL) {
            int j = 0;
            while (j < 64) {
                if (as->pagetable[i][j] == NULL) {
                    continue;
                } else {
                    int k = 0;
                    while (k < 64) {
                        if (as-> pagetable[i][j][k] == 0) {
                            k++;
                            continue;
                        }
                        free_kpages(PADDR_TO_KVADDR(as->pagetable[i][j][k] & PAGE_FRAME));
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
		temp = curr;
		curr = curr->next;
		kfree(temp);
		
	    //temp = curr->next;
		//kfree(curr);
		//curr = temp;	
	}
	
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
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
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

    // 
    as->regions = kmalloc(sizeof(struct region));
    if (as->regions == NULL) return EFAULT;

    // 
    as->regions->flags = as->regions->prev_flags = readable | writeable | executable;
    as->regions->next = NULL;
    //as->regions->npages = memsize / PAGE_SIZE;
    as->regions->npages = memsize;
    as->regions->region_base = vaddr;

    //
    /*struct region *n_region, *c_region, *t_region;
    t_region = c_region = as->regions;
    if (c_region) {
        while(c_region && c_region->start < n_region->start){
            t_region = c_region;
            c_region = c_region->next;
        }
        t_region->next = n_region;
        n_region->next = c_region;
    } else {
        as->regions = n_region;
    }*/

    //
    return 0;

}

/*
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	
	if (as != NULL) {
        memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	    vaddr &= PAGE_FRAME;
	    
	    memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;
	    size_t npages = memsize / PAGE_SIZE;
	    struct region *new;
	    if ((new = kmalloc(sizeof(struct region))) == NULL)
	    {
		    return ENOMEM;
	    }
	    new->vbase = vaddr;
	    new->npages = npages;
	    new->flags = readable | writeable | executable;
	    new->next = NULL;
	    if (as->regions == NULL)
	    {
		    as->regions = new;
	    }
	    else
	    {
		    struct region *cur, *prev;
		    cur = prev = as->regions;
		    while (cur != NULL && cur->vbase < new->vbase)
		    {
			    prev = cur;
			    cur = cur->next;
		    }
		    prev->next = new;
		    new->next = cur;
	    }
	} else {
	    return EFAULT;
	}
	return 0;

*/
/*
	(void)as;
	(void)vaddr;
	(void)memsize;
	(void)readable;
	(void)writeable;
	(void)executable;
	return ENOSYS;  Unimplemented */
//}

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
		if ((oRegions -> flags & PF_W) != PF_W) {
			oRegions -> flags |= PF_W;
		}
		oRegions = oRegions -> next;
	}
	// panic("addrspace: as_prepare_load DONE\n");
	return 0;
	/* 
    struct region * temp = as -> regions;
    while (temp != NULL) {
        temp->prev_flags = temp->flags;
        //temp->flags = 0x7;
        temp->flags = 1;
        temp = temp->next;
    }*/
    
    /*
    if (as != NULL) {
        if (as->regions != NULL) {
            struct region * temp = as -> regions;
            while (temp != NULL) {
                temp->prev_flags = temp->flags;
                //temp->flags = 0x7;
                temp->flags = 1;
                temp = temp->next;
            }
        } else {
            return ENOMEM;
        }
    } else {
        return ENOMEM;    
    }
    */
	//(void)as;
	return 0;
}

struct region *get_region(struct addrspace *as, vaddr_t vaddr) {
    struct region *curr = as->regions;
    while (curr != NULL) {
        if (vaddr >= curr->region_base &&
            (vaddr < (curr->region_base + curr->npages))) {
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
	/*
	 * Write this.
	 */
	/*
    if (as != NULL) {
        if (as->regions != NULL) {
            struct region * temp = as -> regions;
            while (temp != NULL) {
                temp->prev_flags = temp->flags;
                temp = temp->next;
            }
        } else {
            return ENOMEM;
        }
    } else {
        return ENOMEM;    
    }
    */
    if (as == NULL) return EFAULT; // Bad memory reference
    
    as_activate(); //Flush after loading
	
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
					        struct region *region = get_region(as, vaddr);
					        if (region->prev_flags == 0) {
						        pt[i][j][k] = (pt[i][j][k] & PAGE_FRAME) |  TLBLO_VALID;
					        }
				        }
			        }
			    }
				
			}
		}
	}
	
	struct region *curr = as->regions;
	while(curr != NULL) {
		curr->flags = curr->prev_flags;
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

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	int output;
	
	if ((output = as_define_region(as, USERSTACK - 16 * 256, 16 * 256, 1, 1, 0)) != 0) {
	    return output;
	}

	return 0;
}

