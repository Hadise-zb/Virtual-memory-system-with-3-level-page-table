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

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */


struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	as->pagetable = kmalloc(first_level * sizeof (paddr_t **));
	if (as->pagetable == NULL) {
		kfree(as);
		return NULL;
	}
	
	for (int i = 0; i < first_level; i++) {
		as->pagetable[i] = NULL;
	}
	
	as->regions = NULL;

	as->region_flag = 0;

	return as;
}


int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
	
	struct region *old_regions = old->regions;
    struct region *temp = NULL;
    
    while(old_regions != NULL) {
        struct region *new;
        if ((new = kmalloc(sizeof(struct region))) == NULL) {
            as_destroy(newas);
            return ENOMEM;
        }
        new->region_size = old_regions->region_size;
		new->base_size = old_regions->base_size;
		new->flags = old_regions->flags;
		new->next = NULL;
		
		if(temp != NULL) {
			new->next = temp;
			temp = new;
		} else {
			temp = new;
		}
		old_regions = old_regions->next;
    }
    newas->regions = temp;
    int result = PTE_copy(old, newas);
	if ((result != 0)) {
		as_destroy(newas);
		return ENOMEM;
	}

	*ret = newas;
	return 0;
}

int PTE_copy(struct addrspace *old, struct addrspace *newas) {
    for(int i = 0; i < first_level; i++) {
		    if(old->pagetable[i] == NULL){
		        newas->pagetable[i] = NULL;
		        continue;
		    } else {
                
			    newas->pagetable[i] = kmalloc(second_level * sizeof(paddr_t *));
                if (newas->pagetable[i] == NULL) return EFAULT;
                
			    for(int j = 0; j < second_level; j++){
			        if (old->pagetable[i][j] != NULL) {
			            newas->pagetable[i][j] = kmalloc(third_level * sizeof(paddr_t));
			            if (newas->pagetable[i][j] == NULL) return EFAULT;
                        
                        for (int k = 0; k < third_level; k++) {
                            if(old->pagetable[i][j][k] == 0){
                                newas->pagetable[i][j][k] = 0;
                            } else {
					            
					            vaddr_t copyFrame = alloc_kpages(1) & PAGE_FRAME;
					            bzero((void *)copyFrame, PAGE_SIZE);
					            vaddr_t old_base = PADDR_TO_KVADDR(old->pagetable[i][j][k]);
					            memcpy((void *)copyFrame, (const void *)(old_base & PAGE_FRAME), PAGE_SIZE);
                            	newas->pagetable[i][j][k] = (KVADDR_TO_PADDR(copyFrame) & PAGE_FRAME) | (TLBLO_DIRTY & old_base) | (TLBLO_VALID & old_base);
                            }
                        }
                    } else {
                        newas->pagetable[i][j] = NULL; 
                    }
			    }
		    } 
	    }
    return 0;
}


void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	if(as == NULL) return;
		
	for(int i = 0; i < first_level; i++){

		if (as->pagetable[i] == NULL) {
			continue;
		}

		for (int j = 0; j < second_level; j++) {
		    if (as->pagetable[i][j] == NULL) continue;
		    for (int k = 0; k < third_level; k++) {
			    if (as->pagetable[i][j][k] != 0) {
				    free_kpages(PADDR_TO_KVADDR(as->pagetable[i][j][k] & PAGE_FRAME));
			    }
			}
			kfree(as->pagetable[i][j]);
		}

		kfree(as->pagetable[i]);
	}

	kfree(as->pagetable);
		
	struct region *curr = as->regions;
	struct region *temp;
	while (curr != NULL)
	{
		
	    temp = curr->next;
		kfree(curr);
		curr = temp;
	}
	
	kfree(as);
	
}

void
as_activate(void)
{
	struct addrspace *as = proc_getas();
	
	if (as == NULL) return;
	
    int sp = splhigh();
    
    tlb_update();
    
	splx(sp);

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
	/*
	 * Write this.
	 */
	if(as==NULL) return EFAULT;
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;
    struct region *new_region;
	if ((new_region = kmalloc(sizeof(struct region))) == NULL) {
		return ENOMEM;
	}

	initial_region(new_region, readable, writeable, executable, vaddr, memsize);
    
    //insert at the head of linklist
	new_region->next = as->regions;
	as->regions = new_region;

	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	if(as == NULL) return EFAULT;
	
	as->region_flag = TLBLO_DIRTY;
	
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
    if (as == NULL) return EFAULT;
	
	as-> region_flag = 0;
	
	int sp = splhigh();
	
	tlb_update();
	
	splx(sp);

	return 0;

}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	//(void)as;
    *stackptr = USERSTACK;
    
	return as_define_region(as, *stackptr - (16 * PAGE_SIZE), (16 * PAGE_SIZE),1,1,0);
}


void tlb_update(void) {
    for (int j = 0; j < NUM_TLB; j++) {
		tlb_write(TLBHI_INVALID(j),TLBLO_INVALID(),j);
	}
}

void initial_region(region *new_region, int readable, int writeable, int executable, vaddr_t vaddr, size_t memsize) {
    if (executable != 0) {
        new_region->flags = new_region->flags | PF_X;
    }
    if (writeable != 0) {
        new_region->flags = new_region->flags | PF_W;
    }
    if (readable != 0) {
        new_region->flags = new_region->flags | PF_R;
    }
    new_region->base_size = vaddr;
	new_region->region_size = memsize;
}
