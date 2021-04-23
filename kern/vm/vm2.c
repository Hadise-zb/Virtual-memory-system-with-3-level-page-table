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
        
        if (curproc == NULL) return EFAULT; // Bad memory reference
        
        if (faultaddress == 0) return EFAULT; // Bad memory reference
        
        
        if (faulttype != VM_FAULT_READ && faulttype != VM_FAULT_WRITE && faulttype != VM_FAULT_READONLY) {
            return EINVAL;
        } else if (faulttype == VM_FAULT_READONLY) {
            return EFAULT;
        }
        
        if (faultaddress == 0x40000000) return EFAULT;
        
        struct addrspace *as = NULL;
    

        as = proc_getas();
        if (as == NULL) {
            return EFAULT;
        } else if (as-> regions == NULL) {
            return EFAULT;
        } else if (as->pagetable == NULL) {
            return EFAULT;
        } 
        
        uint32_t first_index = faultaddress >> 24;
        uint32_t second_index = (faultaddress << 8) >> 26;
        uint32_t third_index = (faultaddress << 14) >> 26;
        
       

        if (first_index >= 256 || second_index >= 64 || third_index >= 64) {
            return EFAULT;
        }
        
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
        
        uint32_t dirty = 0;
        
        if (as -> pagetable[first_index][second_index][third_index] == 0) {

            struct region *oRegions = as -> regions;
            while (oRegions != NULL) {
                if (faultaddress >= (oRegions -> region_base + oRegions -> npages * PAGE_SIZE) 
                    && faultaddress < oRegions -> region_base) continue;
                
                if ((oRegions -> flags & PF_W) != PF_W) dirty = 0;
                else dirty = TLBLO_DIRTY;
                break;

                oRegions = oRegions -> next;
            }
            if (oRegions == NULL) {
                if (ifinit) kfree(as -> pagetable[first_index]);
                //lock_release(as->lock);
                return EFAULT; // Bad memory reference
            }
            int result = 0;
            vaddr_t region_base = alloc_kpages(1);
            if (region_base == 0) {
                if (ifinit) kfree(as->pagetable[first_index][second_index]);
                //return EFAULT;
                result = ENOMEM;
            }
            bzero((void *)region_base, PAGE_SIZE);
            paddr_t pbase = KVADDR_TO_PADDR(region_base);

            as->pagetable[first_index][second_index][third_index] = (pbase & PAGE_FRAME) | TLBLO_VALID | dirty;

            if (result) {
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
    uint32_t entryhi = faultaddress & TLBHI_VPAGE;
    uint32_t entrylo = as->pagetable[first_index][second_index][third_index];
    int spl = splhigh();  
    tlb_random(entryhi, entrylo);
    splx(spl);
    //lock_release(as->lock);
    return 0;
}


