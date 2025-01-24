#ifndef _CONTROL_REG_H_
#define _CONTROL_REG_H_

#include <kernel.h>

// Function prototypes for reading control registers
unsigned long read_cr0(void);
unsigned long read_cr2(void);
unsigned long read_cr3(void);
unsigned long read_cr4(void);

// Function prototypes for writing control registers
void write_cr0(unsigned long n);
void write_cr3(unsigned long n);
void write_cr4(unsigned long n);

// Function to enable paging
void enable_paging(void);

#endif /* _CONTROL_REG_H_ */
