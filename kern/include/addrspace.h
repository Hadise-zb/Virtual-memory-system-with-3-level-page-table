#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_
#define PAGETABLE_SIZE 1024
/*
 * Address space structure and operations.
 */


#include <vm.h>
#include <synch.h>
#include "opt-dumbvm.h"

struct vnode;


/*
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */

struct addrspace {
#if OPT_DUMBVM
        vaddr_t as_vbase1;
        paddr_t as_pbase1;
        size_t as_npages1;
        vaddr_t as_vbase2;
        paddr_t as_pbase2;
        size_t as_npages2;
        paddr_t as_stackpbase;
#else
        /* Put stuff here for your VM system */
        /* only needs region for addrspace */
        struct region *regions;
	paddr_t ***pagetable;
	struct lock *pt_lock;

#endif
};

/*
 * Regions are code, data, etc.. derived from the ELF file, in KUSEG
 * https://piazza.com/class/jrr04q7mah621s?cid=507
 */

struct region {
	vaddr_t base_addr;
	size_t memsize;
	/* Permissions */
	int readable;
	int writeable;
        int old_writeable;
	int executable;
	struct region *next;
};


struct region * region_create(vaddr_t vaddr, size_t memsize, int readable, int writeable, int executable);
void region_insert(struct addrspace *as, struct region *new);
void region_copy(struct region *old, struct region *new);
int region_valid(struct addrspace *as, vaddr_t vaddr, size_t memsize);
struct region *get_region(struct addrspace *as, vaddr_t vaddr);
void regions_cleanup(struct addrspace *as);
int pt_copy(struct addrspace *old_as, paddr_t **old_pt, paddr_t **new_pt);

int PTE_copy(struct addrspace *old, struct addrspace *newas);
int TLB_f(int spl);
struct region *get_region2(struct addrspace *as, vaddr_t vaddr);
/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make curproc's address space the one currently
 *                "seen" by the processor.
 *
 *    as_deactivate - unload curproc's address space so it isn't
 *                currently "seen" by the processor. This is used to
 *                avoid potentially "seeing" it while it's being
 *                destroyed.
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 *
 * Note that when using dumbvm, addrspace.c is not used and these
 * functions are found in dumbvm.c.
 */

struct addrspace *as_create(void);
int               as_copy(struct addrspace *src, struct addrspace **ret);
void              as_activate(void);
void              as_deactivate(void);
void              as_destroy(struct addrspace *);

int               as_define_region(struct addrspace *as,
                                   vaddr_t vaddr, size_t sz,
                                   int readable,
                                   int writeable,
                                   int executable);
int               as_prepare_load(struct addrspace *as);
int               as_complete_load(struct addrspace *as);
int               as_define_stack(struct addrspace *as, vaddr_t *initstackptr);


/*
 * Functions in loadelf.c
 *    load_elf - load an ELF user program executable into the current
 *               address space. Returns the entry point (initial PC)
 *               in the space pointed to by ENTRYPOINT.
 */

int load_elf(struct vnode *v, vaddr_t *entrypoint);

/* vm.c functions */

int lookup_region(struct addrspace *as, vaddr_t vaddr, int faulttype);
int insert_pt(struct addrspace *as, vaddr_t vaddr, paddr_t paddr);
paddr_t look_up_pt(struct addrspace *as, vaddr_t vaddr);
int probe_pt(struct addrspace *as, vaddr_t vaddr);
int update_pt(struct addrspace *as, vaddr_t addr, paddr_t paddr);
vaddr_t alloc_frame(struct addrspace *as, vaddr_t vaddr);

#endif /* _ADDRSPACE_H_ */
