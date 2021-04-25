#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <current.h>


#include <proc.h>
#include <elf.h>
#include <spl.h>
/* Place your page table functions here */

#define FLC(addr) ((addr) >> (24))
#define SLC(addr) (((addr) << (8)) >> (26))
#define TLC(addr) (((addr) << (14)) >> (26))

// bool
#define FALSE 0
#define TRUE 1

// check authority
#define IFR(flag) ((((flag) & (PF_R)) == (PF_R)) ? (TRUE) : (FALSE))
#define IFW(flag) ((((flag) & (PF_W)) == (PF_W)) ? (TRUE) : (FALSE))
#define IFX(flag) ((((flag) & (PF_X)) == (PF_X)) ? (TRUE) : (FALSE))

/*  rwx = DV
    rw- = DV
    r-x = -V
    r-- = -V
    -wx = DV
    -w- = DV
    --x = -V
    --- = --   */
#define IFD(flag) ((IFW(flag)) ? (TLBLO_DIRTY) : (0))
#define IFV(flag) (((IFR(flag)) || (IFW(flag)) || (IFX(flag))) ? (TLBLO_VALID) : (0))

void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

// check faulttype
int fault_check(int faulttype) {
    if (faulttype == VM_FAULT_READONLY) {
        return EFAULT;
    } else if (faulttype == VM_FAULT_WRITE || faulttype == VM_FAULT_READ) {
        return 0;
    } else {
        return EINVAL;
    }
}

// search a region matched
struct region *region_search(struct addrspace *as, vaddr_t vaddress) {
    struct region *curr = as->regions;
    while (curr != NULL) {
        if (vaddress >= curr->base_size &&
            (vaddress < (curr->base_size + curr->region_size))) {
            return curr;
        }
        curr = curr->next;
    }
    return curr;
}

// load tlb
void TLB_load(uint32_t elo, uint32_t ehi) {
    int spl;
    spl = splhigh();
    tlb_random(ehi, elo);
    splx(spl);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    // faulty type check
    int result = fault_check(faulttype);
    if (result) {
        return result;
    }

    // initial
    if (curproc == NULL) return EFAULT;

    struct addrspace *as = proc_getas();
    if (as == NULL) return EFAULT;

    if (faultaddress >= USERSTACK) return EFAULT;

    struct region *valid_region = region_search(as, faultaddress);
    if (valid_region == NULL) {
        return EFAULT;
    }

    // cut address
    vaddr_t first = FLC(faultaddress);
    vaddr_t second = SLC(faultaddress);
    vaddr_t third = TLC(faultaddress);

    // initial D and V
    uint32_t is_dirty = 0;
    uint32_t is_valid = 0;

    is_dirty |= IFD(valid_region->flags);
    is_valid |= IFV(valid_region->flags);

    // case lvl1 not exits
    if (as->pagetable[first] == NULL) {
        as->pagetable[first] = kmalloc(second_level * sizeof(paddr_t *));

        int i = 0;
        while (i < second_level) {
            as->pagetable[first][i] = NULL;
            i++;
        }
        as->pagetable[first][second] = kmalloc(third_level * sizeof(paddr_t));
        // zero-fill
        i = 0;
        while (i < third_level) {
            as->pagetable[first][second][i] = 0;
            i++;
        }
    }
    // case lvl2 not exits
    else if (as->pagetable[first] != NULL && as->pagetable[first][second] == NULL) {
        as->pagetable[first][second] = kmalloc(third_level * sizeof(paddr_t));
        // zero-fill
        int i = 0;
        while (i < third_level) {
            as->pagetable[first][second][i] = 0;
            i++;
        }
    }

    // pte write in
    if (as->pagetable[first][second][third] == 0) {
        // allocate frame
        vaddr_t newVBase = alloc_kpages(1);
        bzero((void *)newVBase, (size_t)PAGE_SIZE);
        paddr_t newPBase = KVADDR_TO_PADDR(newVBase);
        // insert pagetableE
        as->pagetable[first][second][third] = (newPBase & PAGE_FRAME) | is_dirty | is_valid;
    }

    uint32_t entrylo = as->pagetable[first][second][third] | as->region_flag;
    uint32_t entryhi = faultaddress & PAGE_FRAME;

    // load tlb
    TLB_load(entrylo, entryhi);
    
    // sucess
    return 0;
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
