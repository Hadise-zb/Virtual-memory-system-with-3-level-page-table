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

            vaddr_t addrs = temp->region_base + temp->npages * PAGE_SIZE;
            if (faultaddress < temp->region_base && faultaddress >= addrs) {
                temp = temp->next;
            } else {
                if (temp == NULL) {
                    return EFAULT;
                } else {
                    break;
                }
            }
           
        }


        if (first_index > 256 || second_index > 64 || third_index > 64) {
            return EFAULT;
        }

        int ifinit = 0;

        if (as->pagetable[first_index] == NULL) {
            
            if ((as->pagetable[first_index] = (paddr_t **)kmalloc(sizeof(paddr_t *) * 256)) == NULL) {
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
    
    if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    }
    
    /////
    switch(faulttype) {
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            break;
        case VM_FAULT_READONLY:
            return EFAULT;  // Bad memory reference
        default: 
            return EINVAL;  // invalid arg
    }
    //////
    
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
        as = proc_getas();
        if (as == NULL) {
            return EFAULT;
        } else if (as-> regions == NULL) {
            return EFAULT;
        } else if (as->pagetable == NULL) {
            return EFAULT;
        } else {
            temp = as->regions;
        }

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
        
        temp = as->regions;
        while (temp != NULL) {
            if (faultaddress >= temp->region_base) {
                if (faultaddress - temp->region_base < temp->npages) {
                    break;
                }
            }
            temp = temp->next;
        }
        /*
        if(curr == NULL){
            int stack_size = 16 * PAGE_SIZE;
            // if faultaddress locates in the end of heap and end of stack
            
            if (faultaddress < as->as_stack && faultaddress > (as->as_stack - stack_size)) {
                // dirty 
            }else {
                return EFAULT;
            }
        }*/
        
        //////////////

        if (first_index > 256 || second_index > 64 || third_index > 64) {
            return EFAULT;
        }

        int ifinit = 0;

        if (as->pagetable[first_index] == NULL) {
            
            if ((as->pagetable[first_index] = (paddr_t **)kmalloc(sizeof(paddr_t *) * 256)) == NULL) {
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
        ////////////
        uint32_t dirty = 0;
        
        if (as -> pagetable[first_index][second_index][third_index] == 0) {

            struct region *oRegions = as -> regions;
            while (oRegions != NULL) {
                if (faultaddress >= (oRegions -> region_base + oRegions -> npages * 256) 
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
                return EFAULT; // Bad memory reference
            }
            int result = 0;
            vaddr_t region_base = alloc_kpages(1);
            if (region_base == 0) {
                //if (ifinit) kfree(as->pagetable[first_index][second_index]);
                //return EFAULT;
                result = ENOMEM;
            }
            bzero((void *)region_base, 256);
            paddr_t pbase = KVADDR_TO_PADDR(region_base);

            as->pagetable[first_index][second_index][third_index] = (pbase & PAGE_FRAME) | TLBLO_VALID | dirty;

            //result = vm_addPTE(as -> pagetable, first_index, second_index, third_index, dirty);
            if (result) {
                // if (as -> as_pte[msb] != NULL) kfree(as -> as_pte[msb]);
                if (ifinit) kfree(as -> pagetable[first_index]);
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
