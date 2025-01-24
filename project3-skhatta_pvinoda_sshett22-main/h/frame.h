/* frame.h - Frame Management Header File */

#ifndef _FRAME_H_
#define _FRAME_H_

/* Function Prototypes */
SYSCALL init_frm();
SYSCALL get_frm(int* avail);
SYSCALL free_frm(int i);

#endif /* _FRAME_H_ */