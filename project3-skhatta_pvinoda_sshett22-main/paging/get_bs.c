#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages)
{

    /* requests a mapping of npages pages from backing store bs_id */

    kprintf("Inside get_bs!\n");

    // Validate parameters
    if (bs_id < 0 || bs_id >= MAX_BS || npages <= 0 || npages > 256)
    {
        return SYSERR;
    }

    bs_map_t *bs_entry = &bsm_tab[bs_id];

    // Check if the requested number of pages exceeds the backing store's capacity
    if (npages > BACKING_STORE_UNIT_SIZE / NBPG)
    {
        return SYSERR;
    }

    // If the backing store is unmapped, set its status and maximum number of pages
    if (bs_entry->bs_status == BSM_UNMAPPED)
    {
        bs_entry->bs_status = BSM_MAPPED;
        bs_entry->bs_npages = npages;
    }
    else
    {
        // Backing store is already in use
        // Ensure that the requested npages does not exceed the existing bs_npages
        if (npages > bs_entry->bs_npages)
        {
            return SYSERR;
        }
    }

    // Return the number of pages that can be allocated
    return bs_entry->bs_npages;
}
