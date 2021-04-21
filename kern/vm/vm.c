#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <elf.h>
#include <spl.h>
#include <proc.h>
#include <current.h>
//#include <spinlock.h>
#include <synch.h>
/* Place your page table functions here */


void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

int
vm_fault2(int faulttype, vaddr_t faultaddress)
{
    //if (faultaddress == 0) {
    //        return EFAULT;
    //}
    if (curproc == NULL) return EFAULT; // Bad memory reference
    
    if (faultaddress == 0) return EFAULT; // Bad memory reference
    
    if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    }
    
    struct addrspace *as = NULL;
    
    int first_index = (int)faultaddress >> 24;
    int second_index = (int)(faultaddress >> 18) & 0b111111;
    int third_index = (int)(faultaddress >> 12) & 0b111111;
    
    if ((faulttype == VM_FAULT_WRITE) || (faulttype == VM_FAULT_READ)) {
        if (curproc == NULL) {
            return EFAULT;
        }
        faultaddress &= PAGE_FRAME;
        
        //struct addrspace *as;
        struct region * temp;
        if ((as = proc_getas()) == NULL) {
            return EFAULT;
        } else if (as-> regions == NULL) {
            return EFAULT;
        } else {
            temp = as->regions;
        }

        // find fault
        while (temp != NULL) {

            //vaddr_t addrs = temp->region_base + temp->npages * PAGE_SIZE;
            vaddr_t addrs = temp->region_base + temp->npages;
            if (faultaddress < temp->region_base && faultaddress >= addrs) {
                //temp = temp->next;
            } else {
                if (temp == NULL) {
                    return EFAULT;
                } else {
                    break;
                }
            }
            temp = temp->next;
        }
        if (temp == NULL) {
            return EFAULT;
        }

        if (first_index > 256 || second_index > 64 || third_index > 64) {
            return EFAULT;
        }

        int ifinit = 0;

        if (as->pagetable[first_index] == NULL) {
            
            if ((as->pagetable[first_index] = (paddr_t **)kmalloc(sizeof(paddr_t *) * PAGE_SIZE)) == NULL) {
                return ENOMEM;
            }
            int i = 0;
            
            
            while (i < 64) {
                as->pagetable[first_index][i] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
                if (as->pagetable[first_index][i] == NULL) {
                    return ENOMEM;
                }
                for (int j = 0; j < 64; j++)
                {
                    as->pagetable[first_index][i][j] = 0;
                }
                i++;
            }
            ifinit = 1;
        }
        
        else if (as->pagetable[first_index] != NULL && as->pagetable[first_index][second_index] == NULL) {
            as->pagetable[first_index][second_index] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
            if (as->pagetable[first_index][second_index] == NULL) {
                return ENOMEM;
            }
            for (int j = 0; j < 64; j++)
            {
                as->pagetable[first_index][second_index][j] = 0;
            }
            ifinit = 1;
        }

        struct region *reg = as->regions;
        while (reg != NULL) {
            if (faultaddress < (reg -> region_base + reg -> npages * 256) || faultaddress >= reg -> region_base) break;
            reg = reg->next;
        }

        int dirty = ((reg -> flags & PF_W) != PF_W) ? 1 : 0;

        vaddr_t region_base = alloc_kpages(1);
        if (region_base == 0) {
            if (ifinit) kfree(as->pagetable[first_index][second_index]);
            return EFAULT;
        }
        bzero((void *)region_base, PAGE_SIZE);
        paddr_t pbase = KVADDR_TO_PADDR(region_base);

        as->pagetable[first_index][second_index][third_index] = (pbase & PAGE_FRAME) | TLBLO_VALID | dirty;

        /*if (as->pagetable[first_index][second_index][third_index] == 0) {
            as->pagetable[first_index][second_index][third_index] = KVADDR_TO_PADDR(alloc_kpages(1));
            bzero((void *)PADDR_TO_KVADDR(as->pagetable[root_index][sec_index][third_index]), 64);
        }
        paddr_t paddr = as->pagetable[root_index][second_index][];*/

    } else if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    } else {
        return EINVAL;
    }

     /* Entry high is the virtual address */
    //uint32_t entryhi = faultaddress & TLBHI_VPAGE;
    uint32_t entryhi = faultaddress & PAGE_FRAME;
    /* Entry low is physical frame, dirty bit, and valid bit. */
    uint32_t entrylo = as->pagetable[first_index][second_index][third_index];
    /* Disable interrupts on this CPU while frobbing the TLB. */
    int spl = splhigh();
    /* Randomly add pagetable entry to the TLB. */
    tlb_random(entryhi, entrylo);
    splx(spl);
    return 0;

    //(void) faulttype;
    //(void) fault
    
    //(void) faultaddress;

    //panic("vm_fault hasn't been written yet\n");

    //return EFAULT;
}
/*
 * SMP-specific functions.  Unused in our UNSW configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

/////////////////////////////////////////////////////

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
        //if (faultaddress == 0) {
        //        return EFAULT;
        //}
        if (curproc == NULL) return EFAULT; // Bad memory reference
        
        if (faultaddress == 0) return EFAULT; // Bad memory reference
        
        //if (faultaddress <= 0x7fffffff) return EFAULT;
        //if (faultaddress == 0x40000000) return EFAULT;
        /////
        //if (faultaddress < (0x80000000 - (16 * PAGE_SIZE))) return EFAULT;
        //if ((faultaddress & PAGE_FRAME) > 16 * PAGE_SIZE) return EFAULT;
        switch(faulttype) {
            case VM_FAULT_READ:
                break;
            case VM_FAULT_WRITE:
                break;
            case VM_FAULT_READONLY:
                return EFAULT;  // Bad memory reference
            default: 
                return EINVAL;  // invalid arg
        }
        //////
        
        
        
        
        struct addrspace *as = NULL;
    
    
    
        
        //faultaddress &= PAGE_FRAME;
        
        //struct addrspace *as;
        //struct region * temp;
        as = proc_getas();
        if (as == NULL) {
            return EFAULT;
        } else if (as-> regions == NULL) {
            return EFAULT;
        } else if (as->pagetable == NULL) {
            return EFAULT;
        } 
        /*
        struct region *curr = as->regions;
        while(curr != NULL) {
            if (faultaddress >= curr->region_base &&
                (faultaddress < (curr->region_base + curr->npages))) {
                break;
            }
            curr = curr->next;
        }

        if (curr == NULL) return EFAULT;
        */
        /*
        temp = as->regions;
        while (temp != NULL) {
            if (faultaddress >= temp->region_base) {
                if (faultaddress - temp->region_base < temp->npages) {
                    break;
                }
            }
            temp = temp->next;
        }
        
        if(temp == NULL){
            int stack_size = 16 * PAGE_SIZE;
            // if faultaddress locates in the end of heap and end of stack
            
            if (faultaddress < USERSTACK && faultaddress > (USERSTACK - stack_size)) {
                // dirty 
            }else {
                return EFAULT;
            }
        }*/
        //temp = as->regions;
        

        // find fault
        /*
        while (temp != NULL) {

            vaddr_t addrs = temp->region_base + temp->npages * 256;
            if (faultaddress < temp->region_base && faultaddress >= addrs) {
                temp = temp->next;
            } else {
                if (temp == NULL) {
                    return EFAULT;
                } else {
                    break;
                }
            }
           
        }*/
        uint32_t first_index = faultaddress >> 24;
        uint32_t second_index = (faultaddress << 8) >> 26;
        uint32_t third_index = (faultaddress << 14) >> 26;
        /*
        int first_index, second_index, third_index;
        first_index = (int)faultaddress >> 24;
        second_index = (int)faultaddress << 8;
        second_index = (int)second_index >> 24;
        third_index = (int)faultaddress << 14;
        third_index = (int)third_index >> 24;
        */
        //////////////

        if (first_index >= 256 || second_index >= 64 || third_index >= 64) {
            return EFAULT;
        }
        /*
        if (probe_pt(as, faultaddress) == 0)
        {
            
            if (lookup_region(as, faultaddress, faulttype) == 0) {
                
                //load_tlb(faultaddress & PAGE_FRAME, as->pagetable[first_index][second_index][third_index]);
                uint32_t entryhi = faultaddress & TLBHI_VPAGE;
                
                uint32_t entrylo = as->pagetable[first_index][second_index][third_index];
               
                int spl = splhigh();
                
                tlb_random(entryhi, entrylo);
                splx(spl);
                return 0; 
            }
            return EFAULT;
        }*/
        
        /*
        while (temp != NULL) {

            //vaddr_t addrs = temp->region_base + temp->npages * PAGE_SIZE;
            vaddr_t addrs = temp->region_base + temp->npages;
            if (faultaddress < temp->region_base && faultaddress >= addrs) {
                //temp = temp->next;
            } else {
                if (temp == NULL) {
                    return EFAULT;
                } else {
                    break;
                }
            }
            temp = temp->next;
        }
        if (temp == NULL) {
            return EFAULT;
        }
        
        */
        
        
        
        
        //paddr_t pbase = KVADDR_TO_PADDR(faultaddress);
        //uint32_t first_index = (int)faultaddress >> 24;
        //uint32_t second_index = (int)(faultaddress >> 18) & 0b111111;
        //uint32_t third_index = (int)(faultaddress >> 12) & 0b111111;
        
        
        //lock_acquire(as->lock);
        int ifinit = 0;
        
        //lock_acquire(as->lock);
        
        if (as->pagetable[first_index] == NULL) {
            
            if ((as->pagetable[first_index] = (paddr_t **)kmalloc(sizeof(paddr_t *) * 256)) == NULL) {
                //lock_release(as->lock);
                return ENOMEM;
            }
            int i = 0;
            
            
            while (i < 64) {
                as->pagetable[first_index][i] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
                if (as->pagetable[first_index][i] == NULL) {
                    // delete the space before
                    for (int g = 0; g < i; g++) {
                        kfree(as->pagetable[first_index][g]);
                    }
                    //lock_release(as->lock);
                    return ENOMEM;
                }
                for (int j = 0; j < 64; j++)
                {
                    as->pagetable[first_index][i][j] = 0;
                }
                i++;
            }
            ifinit = 1;
        }
        
        if (as->pagetable[first_index] != NULL && as->pagetable[first_index][second_index] == NULL) {
            as->pagetable[first_index][second_index] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
            if (as->pagetable[first_index][second_index] == NULL) {
                //lock_release(as->lock);
                return ENOMEM;
            }
            for (int j = 0; j < 64; j++)
            {
                as->pagetable[first_index][second_index][j] = 0;
            }
            ifinit = 1;
        }
        ////////////
        uint32_t dirty = 0;
        
        if (as -> pagetable[first_index][second_index][third_index] == 0) {

            struct region *oRegions = as -> regions;
            while (oRegions != NULL) {
                if (faultaddress >= (oRegions -> region_base + oRegions -> npages * PAGE_SIZE) 
                    && faultaddress < oRegions -> region_base) continue;
                // READONLY
                if ((oRegions -> flags & PF_W) != PF_W) dirty = 0;
                else dirty = TLBLO_DIRTY;
                break;

                oRegions = oRegions -> next;
            }
            if (oRegions == NULL) {
                // if (as -> as_pte[msb] != NULL) kfree(as -> as_pte[msb]);
                if (ifinit) kfree(as -> pagetable[first_index]);
                //lock_release(as->lock);
                return EFAULT; // Bad memory reference
            }
            int result = 0;
            vaddr_t region_base = alloc_kpages(1);
            if (region_base == 0) {
                //if (ifinit) kfree(as->pagetable[first_index][second_index]);
                //return EFAULT;
                result = ENOMEM;
            }
            bzero((void *)region_base, PAGE_SIZE);
            paddr_t pbase = KVADDR_TO_PADDR(region_base);

            as->pagetable[first_index][second_index][third_index] = (pbase & PAGE_FRAME) | TLBLO_VALID | dirty;

            //result = vm_addPTE(as -> pagetable, first_index, second_index, third_index, dirty);
            if (result) {
                // if (as -> as_pte[msb] != NULL) kfree(as -> as_pte[msb]);
                if (ifinit) {
                    for (int h = 0; h < 64; h++) {
                        kfree(as -> pagetable[first_index][h]);
                    }
                    kfree(as->pagetable[first_index]);
                }
                //lock_release(as->lock);
                return result;
            }
        }
     ///////   
        /*
        struct region *reg = as->regions;
        while (reg != NULL) {
            if (faultaddress < (reg -> region_base + reg -> npages * 256) || faultaddress >= reg -> region_base) break;
            reg = reg->next;
        }
        
        //
         if (reg == NULL) {
            // if (as -> as_pte[msb] != NULL) kfree(as -> as_pte[msb]);
            if (ifinit) kfree(as -> pagetable[first_index]);
            return EFAULT; // Bad memory reference
        }
        //
        
        int dirty = ((reg -> flags & PF_W) != PF_W) ? 1 : 0;

        vaddr_t region_base = alloc_kpages(1);
        if (region_base == 0) {
            if (ifinit) kfree(as->pagetable[first_index][second_index]);
            return EFAULT;
        }
        bzero((void *)region_base, PAGE_SIZE);
        paddr_t pbase = KVADDR_TO_PADDR(region_base);

        as->pagetable[first_index][second_index][third_index] = (pbase & PAGE_FRAME) | TLBLO_VALID | dirty;
        */
        /*if (as->pagetable[first_index][second_index][third_index] == 0) {
            as->pagetable[first_index][second_index][third_index] = KVADDR_TO_PADDR(alloc_kpages(1));
            bzero((void *)PADDR_TO_KVADDR(as->pagetable[root_index][sec_index][third_index]), 64);
        }
        paddr_t paddr = as->pagetable[root_index][second_index][];*/
    //lock_release(as->lock);
    

     /* Entry high is the virtual address */
    //uint32_t entryhi = faultaddress & TLBHI_VPAGE;
    uint32_t entryhi = faultaddress & TLBHI_VPAGE;
    /* Entry low is physical frame, dirty bit, and valid bit. */
    uint32_t entrylo = as->pagetable[first_index][second_index][third_index];
    /* Disable interrupts on this CPU while frobbing the TLB. */
    int spl = splhigh();
    /* Randomly add pagetable entry to the TLB. */
    tlb_random(entryhi, entrylo);
    splx(spl);
    //lock_release(as->lock);
    return 0;

    //(void) faulttype;
    //(void) fault
    
    //(void) faultaddress;

    //panic("vm_fault hasn't been written yet\n");

    //return EFAULT;
}


/////////////////
/*
paddr_t look_up_pt(struct addrspace *as, vaddr_t vaddr)
{
    KASSERT(0);
    uint32_t pd_bits = get_first_10_bits(vaddr);
    uint32_t pt_bits = get_middle_10_bits(vaddr);

    if (probe_pt(as, vaddr) != 0) {
        return EMPTY; //TODO: REPLACE with OUT OF RANGE
    }

    pd_bits = get_first_10_bits(vaddr);
    pt_bits = get_middle_10_bits(vaddr);

    return as->pagetable[pd_bits][pt_bits];
}

paddr_t look_up_pt(struct addrspace *as, vaddr_t vaddr)
{
    KASSERT(0);
    uint32_t pd_bits = get_first_10_bits(vaddr);
    uint32_t pt_bits = get_middle_10_bits(vaddr);

    if (probe_pt(as, vaddr) != 0) {
        return EMPTY; //TODO: REPLACE with OUT OF RANGE
    }

    pd_bits = get_first_10_bits(vaddr);
    pt_bits = get_middle_10_bits(vaddr);

    return as->pagetable[pd_bits][pt_bits];
}

int probe_pt(struct addrspace *as, vaddr_t vaddr) {
    uint32_t pd_bits = get_first_10_bits(vaddr);
    uint32_t pt_bits = get_middle_10_bits(vaddr);

    if (pd_bits >= PAGETABLE_SIZE || pt_bits >= PAGETABLE_SIZE) {
        return -1; //TODO: REPLACE with OUT OF RANGE
    }
    lock_acquire(as->pt_lock);
    if (as->pagetable == NULL) {
        lock_release(as->pt_lock);
        return -1;
    }
    if (as->pagetable[pd_bits] == NULL) {
        lock_release(as->pt_lock);
        return -1;
    }

    if (as->pagetable[pd_bits][pt_bits] == EMPTY) {
        lock_release(as->pt_lock);
        return -1;
    }

    lock_release(as->pt_lock);
    
    return 0;
}

void load_tlb(uint32_t entryhi, uint32_t entrylo)
{
    int sql = splhigh();
    tlb_random(entryhi, entrylo);
    splx(sql);
}


int lookup_region(struct addrspace *as, vaddr_t vaddr, int faulttype)
{
    struct region *curr = as->head;
    while(curr != NULL) {
        if (vaddr >= curr->base_addr &&
            (vaddr < (curr->base_addr + curr->memsize))) {
            break;
        }
        curr = curr->next;
    }

    if (curr == NULL)     return EFAULT; 

    switch (faulttype)
    {
        case VM_FAULT_WRITE:
            if (curr->writeable == 0) return EPERM;
            break;
        case VM_FAULT_READ:
            if (curr->readable == 0) return EPERM;
            break;
        default:
            return EINVAL; 
    }
    return 0;
}

struct region *get_region(struct addrspace *as, vaddr_t vaddr) {
    struct region *curr = as->head;
    while (curr != NULL) {
        if (vaddr >= curr->base_addr &&
            (vaddr < (curr->base_addr + curr->memsize))) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

vaddr_t get_first_10_bits(vaddr_t addr) {
    //return KVADDR_TO_PADDR(addr) >> 22;
    return addr >> 22;
}

vaddr_t get_middle_10_bits(vaddr_t addr) {
    //return (KVADDR_TO_PADDR(addr) << 10) >> 22;
    return (addr << 10) >> 22;
}


vaddr_t alloc_frame(struct addrspace *as, vaddr_t vaddr) {
	
    (void)as;
    vaddr = vaddr;
	//struct region *region = get_region(as, vaddr);
	//size_t regionSize = region->memsize;
	
	//unsigned int frameNeeded = regionSize / PAGE_SIZE;
	//vaddr_t newVaddr = alloc_kpages(frameNeeded);
    vaddr_t newVaddr = alloc_kpages(1);
	if (newVaddr == 0) return 0;
	
	//bzero((void *) newVaddr, regionSize);
    bzero((void *) newVaddr, PAGE_SIZE);
	return newVaddr;
}

int update_pt(struct addrspace *as, vaddr_t vaddr, paddr_t paddr)
{
    KASSERT(0);
    uint32_t result = probe_pt(as, vaddr);
    if (result) return result;

    uint32_t pd_bits = get_first_10_bits(vaddr);
    uint32_t pt_bits = get_middle_10_bits(vaddr);

    as->pagetable[pd_bits][pt_bits] = paddr;

    return 0;
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
    

    switch (faulttype)
    {
        case VM_FAULT_READONLY:
            return EFAULT;
        case VM_FAULT_WRITE:
        case VM_FAULT_READ:
            break;
        default:
            return EINVAL; 
    }
    if (curproc == NULL) {
        
        return EFAULT;
    }



    struct addrspace *as = proc_getas(); 
    if (as == NULL) {
        
        return EFAULT;
    }
    paddr_t **pagetable = as->pagetable;
    if (pagetable == NULL) return EFAULT;
    uint32_t pd_bits = get_first_10_bits(faultaddress);
    uint32_t pt_bits = get_middle_10_bits(faultaddress);



    
    if (probe_pt(as, faultaddress) == 0)
    {
        
        if (lookup_region(as, faultaddress, faulttype) == 0) {
            
            load_tlb(faultaddress & PAGE_FRAME, pagetable[pd_bits][pt_bits]);
            return 0;
        }
        return EFAULT;
    }


    
    int result = lookup_region(as, faultaddress, faulttype); 
    if(result) {
        return result;
    }

    
    vaddr_t newVaddr = alloc_frame(as, faultaddress);
    if (newVaddr == 0) return ENOMEM;
    paddr_t paddr = KVADDR_TO_PADDR(newVaddr) & PAGE_FRAME;
    struct region *region = get_region(as, faultaddress);
    
    if (region->writeable != 0) {
        paddr = paddr | TLBLO_DIRTY;
    }

    result = insert_pt(as, faultaddress, paddr | TLBLO_VALID);
    if (result != 0) {
        return ENOMEM;
    }
    
    load_tlb(faultaddress & PAGE_FRAME, pagetable[pd_bits][pt_bits]);
    return 0;         

}
*/


/* Place your page table functions here */
vaddr_t first_lvl_cut(vaddr_t addr) {
    return addr >> 24;
}

vaddr_t second_lvl_cut(vaddr_t addr) {
    return (addr << 8) >> 26;
}

vaddr_t third_lvl_cut(vaddr_t addr) {
    return (addr << 14) >> 26;
}

void load_tlb(uint32_t entryhi, uint32_t entrylo)
{
    int sql = splhigh();
    tlb_random(entryhi, entrylo);
    splx(sql);
}

int insert_pt(struct addrspace *as, vaddr_t vaddr, paddr_t paddr)
{
    uint32_t first_index = first_lvl_cut(vaddr);
    uint32_t second_index = second_lvl_cut(vaddr);
    uint32_t third_index = third_lvl_cut(vaddr);

    if (first_index >= 256 || second_index >= 64 || third_index >= 64) {
        return EFAULT;
    }

    //lock_acquire(as->pt_lock);

    if (as->pagetable[first_index] == NULL) {
        as->pagetable[first_index] = (paddr_t **)kmalloc(sizeof(paddr_t *) * 64);
        if (as->pagetable[first_index] == NULL) {
            //lock_release(as->pt_lock);
            return ENOMEM;
        }
        as->pagetable[first_index][second_index] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
        if (as->pagetable[first_index][second_index] == NULL) {
            //kfree(as->pagetable[first_index]);
            //lock_release(as->pt_lock);
            return ENOMEM;
        }
        for (int i = 0; i < 64; i++) {
            as->pagetable[first_index][second_index][i] = 0;
        }
    }
    else if (as->pagetable[first_index] != NULL && as->pagetable[first_index][second_index] == NULL) {
        as->pagetable[first_index][second_index] = (paddr_t *)kmalloc(sizeof(paddr_t) * 64);
        if (as->pagetable[first_index][second_index] == NULL) {
            //kfree(as->pagetable[first_index]);
            //lock_release(as->pt_lock);
            return ENOMEM;
        }
        for (int i = 0; i < 64; i++) {
            as->pagetable[first_index][second_index][i] = 0;
        }
    }
    else if (as->pagetable[first_index][second_index][third_index] != 0) {
        //kfree(as->pagetable[first_index][second_index]);
        //kfree(as->pagetable[first_index]);
        //lock_release(as->pt_lock);
        return EFAULT;
    }

    as->pagetable[first_index][second_index][third_index] = paddr;
    //lock_release(as->pt_lock);

    return 0;
}

paddr_t look_up_pt(struct addrspace *as, vaddr_t vaddr)
{
    //KASSERT(0);
    uint32_t first_index = first_lvl_cut(vaddr);
    uint32_t second_index = second_lvl_cut(vaddr);
    uint32_t third_index = third_lvl_cut(vaddr);

    if (probe_pt(as, vaddr) != 0) {
        return 0;
    }

    first_index = first_lvl_cut(vaddr);
    second_index = second_lvl_cut(vaddr);
    third_index = third_lvl_cut(vaddr);

    return as->pagetable[first_index][second_index][third_index];
}

int probe_pt(struct addrspace *as, vaddr_t vaddr) {
    uint32_t first_index = first_lvl_cut(vaddr);
    uint32_t second_index = second_lvl_cut(vaddr);
    uint32_t third_index = third_lvl_cut(vaddr);

    if (first_index >= 256 || second_index >= 64 || third_index >= 64) {
        return -1;
    }
    //lock_acquire(as->pt_lock);
    if (as->pagetable == NULL) {
        //lock_release(as->pt_lock);
        return -1;
    }
    if (as->pagetable[first_index] == NULL) {
        //lock_release(as->pt_lock);
        return -1;
    }

    if (as->pagetable[first_index][second_index] == NULL) {
        //lock_release(as->pt_lock);
        return -1;
    }

    if (as->pagetable[first_index][second_index][third_index] == 0) {
        //lock_release(as->pt_lock);
        return -1;
    }

    //lock_release(as->pt_lock);
    
    return 0;
}

int update_pt(struct addrspace *as, vaddr_t vaddr, paddr_t paddr)
{
    //KASSERT(0);
    uint32_t result = probe_pt(as, vaddr);
    if (result) return result;

    uint32_t first_index = first_lvl_cut(vaddr);
    uint32_t second_index = second_lvl_cut(vaddr);
    uint32_t third_index = third_lvl_cut(vaddr);

    as->pagetable[first_index][second_index][third_index] = paddr;

    return 0;
}

int lookup_region(struct addrspace *as, vaddr_t vaddr, int faulttype)
{
    struct region *curr = as->regions;
    while(curr != NULL) {
        if (vaddr >= curr->region_base &&
            (vaddr < (curr->region_base + curr->npages))) {
            break;
        }
        curr = curr->next;
    }

    if (curr == NULL) return EFAULT;

    switch (faulttype)
    {
        case VM_FAULT_WRITE:
            if ((curr->flags & PF_W) != PF_W) return EPERM;
            break;
        case VM_FAULT_READ:
            if ((curr->flags & PF_R) != PF_R) return EPERM;
            break;
        default:
            return EINVAL;
    }
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

////



int
vm_fault3(int faulttype, vaddr_t faultaddress)
{
    switch (faulttype)
    {
        case VM_FAULT_READONLY:
            return EFAULT;
        case VM_FAULT_WRITE:
        case VM_FAULT_READ:
            break;
        default:
            return EINVAL;
    }
    if (curproc == NULL) {
        return EFAULT;
    }



    struct addrspace *as = proc_getas();
    if (as == NULL) {
        return EFAULT;
    }
    paddr_t ***pagetable = as->pagetable;
    if (pagetable == NULL) return EFAULT;
    uint32_t first_index = first_lvl_cut(faultaddress);
    uint32_t second_index = second_lvl_cut(faultaddress);
    uint32_t third_index = third_lvl_cut(faultaddress);



    /* Look up Page Table */
    if (probe_pt(as, faultaddress) == 0)
    {
        /* get region and check bits */
        if (lookup_region(as, faultaddress, faulttype) == 0) {
            /* Load into TLB */
            load_tlb(faultaddress & PAGE_FRAME, pagetable[first_index][second_index][third_index]);
            return 0;
        }
        return EFAULT;
    }


    /* Check for valid Region
     * This means that we have to check whether its accessing a page that
     * should exist but have not been accessed yet
     */
    int result = lookup_region(as, faultaddress, faulttype); 
    if(result) {
        return result;
    }

    /* Found a region that is within process's region*/
    vaddr_t newVaddr = alloc_frame(as, faultaddress);
    if (newVaddr == 0) return ENOMEM;
    paddr_t paddr = KVADDR_TO_PADDR(newVaddr) & PAGE_FRAME;
    struct region *region = get_region(as, faultaddress);
    /* insert into PTE */
    if ((region->flags & PF_W) != PF_W) {
        paddr = paddr | TLBLO_DIRTY;
    }

    result = insert_pt(as, faultaddress, paddr | TLBLO_VALID);
    if (result != 0) {
        return ENOMEM;
    }
    /* Load TLB*/
    load_tlb(faultaddress & PAGE_FRAME, pagetable[first_index][second_index][third_index]);
    return 0;
}



