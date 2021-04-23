#ifndef _VM_H_
#define _VM_H_

/*
 * VM system-related definitions.
 */
#define USER_STACK_SIZE 16 * PAGE_SIZE /* defined in spec */



#include <machine/vm.h>
/*
 * Added directives
 */
#include <addrspace.h>


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages);
void free_kpages(vaddr_t addr);

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown(const struct tlbshootdown *);



#endif /* _VM_H_ */
