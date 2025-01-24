/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL vfreemem(struct mblock *block, unsigned size)
{
    STATWORD ps;
    struct mblock *p, *q;
    unsigned int block_start, block_end;
    unsigned int vheap_start, vheap_end;

    disable(ps);

    /* Validate the size of the block*/
    if (size == 0 || (unsigned)block < VHEAP_START * NBPG)
    {
        restore(ps);
        return SYSERR;
    }

    /* Round size to memory block size */
    size = (unsigned int)roundmb(size);

    /* Get the start and end addresses of the virtual heap */
    vheap_start = proctab[currpid].vhpno * NBPG;
    vheap_end = vheap_start + proctab[currpid].vhpnpages * NBPG;

    /* Get the start and end addresses of the block to free */
    block_start = (unsigned int)block;
    block_end = block_start + size;

    /* Validate that the block is within the process's virtual heap */
    if (block_start < vheap_start || block_end > vheap_end)
    {
        restore(ps);
        return SYSERR;
    }

    /* Initialize pointers to traverse the free list */
    q = proctab[currpid].vmemlist; /* Head of the free list */
    p = q->mnext;

    /* Traverse the free list to find the correct position to insert */
    while (p != NULL && p < block)
    {
        q = p;
        p = p->mnext;
    }

    /* Check for overlap with adjacent blocks */
    if ((q != proctab[currpid].vmemlist && (unsigned int)q + q->mlen > block_start) ||
        (p != NULL && block_end > (unsigned int)p))
    {
        restore(ps);
        return SYSERR;
    }

    /* Debugged this part if any trap found */
    /* Merge with previous block if adjacent */
    if (q != proctab[currpid].vmemlist && (unsigned int)q + q->mlen == block_start)
    {
        q->mlen += size;
        /* Merge with next block if adjacent */
        if (p != NULL && block_end == (unsigned int)p)
        {
            q->mlen += p->mlen;
            q->mnext = p->mnext;
        }
    }
    else
    {
        /* Merge with next block if adjacent */
        if (p != NULL && block_end == (unsigned int)p)
        {
            block->mlen = size + p->mlen;
            block->mnext = p->mnext;
        }
        else
        {
            /* No adjacent blocks, insert the block */
            block->mlen = size;
            block->mnext = p;
        }
        q->mnext = block;
        // q = block;
    }

    restore(ps);
    return OK;
}