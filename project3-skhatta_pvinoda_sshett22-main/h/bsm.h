/* bsm.h - Backing Store Management Header File */

#ifndef _BSM_H_
#define _BSM_H_

/* Function Prototypes */
SYSCALL init_bsm();
SYSCALL get_bsm(int* avail);
SYSCALL free_bsm(int i);
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth);
SYSCALL bsm_map(int pid, int vpno, int source, int npages);
SYSCALL bsm_unmap(int pid, int vpno, int flag);

#endif /* _BSM_H_ */