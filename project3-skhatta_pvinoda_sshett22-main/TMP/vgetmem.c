#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];

/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD *vgetmem(unsigned nbytes)
{
    STATWORD ps;
    struct mblock *p, *q, *leftover;
    unsigned int block_size;

    disable(ps);

    /* Check if nbytes is valid and if the free list is not empty */
    if (nbytes == 0 || proctab[currpid].vmemlist->mnext == NULL)
    {
        restore(ps);
        return (WORD *)SYSERR;
    }

    /* Round nbytes up to the nearest multiple of memory block size */
    nbytes = (unsigned int)roundmb(nbytes);

    /* Initialize pointers to traverse the free list */
    q = proctab[currpid].vmemlist;
    p = q->mnext;

    /* Traverse the free list to find a suitable block */
    while (p != NULL)
    {
        block_size = p->mlen;
        if (block_size == nbytes)
        {
            /* Found an exact fit block size */
            q->mnext = p->mnext;
            restore(ps);
            return (WORD *)p;
        }
        else if (block_size > nbytes)
        {
            /* Allocate portion of the block */
            leftover = (struct mblock *)((unsigned int)p + nbytes);
            leftover->mlen = block_size - nbytes;
            leftover->mnext = p->mnext;

            q->mnext = leftover;

            p->mlen = nbytes; /* Included */
            restore(ps);
            return (WORD *)p;
        }
        else
        {
            q = p;
            p = p->mnext;
        }
    }

    /* No suitable block is found */
    restore(ps);
    return (WORD *)SYSERR;
}