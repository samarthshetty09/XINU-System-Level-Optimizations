#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

SYSCALL write_bs(char *src, bsd_t bs_id, int page)
{
    // Validate inputs
    if (bs_id < 0 || bs_id >= MAX_BS)
    {
        return SYSERR;
    }

    if (page < 0 || page >= 256)
    {
        return SYSERR;
    }

    bs_map_t *bs_entry = &bsm_tab[bs_id];

    if (bs_entry->bs_status == BSM_UNMAPPED)
    {
        return SYSERR; // Cannot write to an unmapped backing store
    }

    // Check if the current process has a mapping to this backing store
    int pid = currpid;
    int mapping_found = 0;
    int i;
    for (i = 0; i < MAX_BSM_MAPPINGS; i++)
    {
        if (bs_entry->mappings[i].pid == pid)
        {
            // Optionally, check if the page number is within the allocated range
            int npages = bs_entry->mappings[i].npages;
            if (page < npages)
            {
                mapping_found = 1;
                break;
            }
            else
            {
                return SYSERR; // Page number out of bounds for this mapping
            }
        }
    }

    if (!mapping_found)
    {
        return SYSERR; // Current process does not have a mapping to this backing store
    }

    // Calculate physical address
    char *phy_addr = (char *)(BACKING_STORE_BASE + bs_id * BACKING_STORE_UNIT_SIZE + page * NBPG);

    // Write to backing store from source
    bcopy((void *)src, phy_addr, NBPG);

    return OK;
}