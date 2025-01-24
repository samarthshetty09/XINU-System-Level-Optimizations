#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id)
{

    /* Release the backing store with ID bs_id */
    kprintf("Inside release_bs. About to release backing store %d\n", bs_id);

    STATWORD ps;
    disable(ps);

    if (bs_id < 0 || bs_id >= MAX_BS)
    {
        restore(ps);
        return SYSERR;
    }

    bs_map_t *bs_entry = &bsm_tab[bs_id]; 

    if (bs_entry->bs_status == BSM_UNMAPPED)
    {
        restore(ps);
        return SYSERR; 
    }
    int i;
    // checking for exzisting mappings 
    int mappings_exist = 0;
    for (i = 0; i < MAX_BSM_MAPPINGS; i++)
    {
        if (bs_entry->mappings[i].pid != -1)
        {
            mappings_exist = 1;
            break;
        }
    }

    if (mappings_exist)
    {
        kprintf("release_bs: Cannot release backing store %d; mappings still exist.\n", bs_id);
        restore(ps);
        return SYSERR;
    }

    bs_entry->bs_status = BSM_UNMAPPED; 
    bs_entry->bs_npages = 0;            
    bs_entry->bs_sem = 0;               

    for (i = 0; i < MAX_BSM_MAPPINGS; i++)
    {
        bs_entry->mappings[i].pid = -1;
        bs_entry->mappings[i].vpno = 0;
        bs_entry->mappings[i].npages = 0;
    }

    restore(ps);
    return OK;
}
