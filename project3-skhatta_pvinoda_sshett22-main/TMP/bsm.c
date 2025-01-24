/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
/* Define the backing store map table */
bs_map_t bsm_tab[MAX_BS];
SYSCALL init_bsm()
{
    int i, j;
    for (i = 0; i < MAX_BS; i++)
    {
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].bs_private = 0;
        for (j = 0; j < MAX_BSM_MAPPINGS; j++)
        {
            bsm_tab[i].mappings[j].pid = -1;
            bsm_tab[i].mappings[j].vpno = 0;
            bsm_tab[i].mappings[j].npages = 0;
        }
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int *avail)
{
    int i;
    for (i = 0; i < MAX_BS; i++)
    {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED)
        {
            *avail = i;
            return OK;
        }
    }
    return SYSERR; // No free backing store available
}

/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    if (i < 0 || i >= MAX_BS)
    {
        return SYSERR; // Invalid backing store ID
    }
    bsm_tab[i].bs_status = BSM_UNMAPPED;
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_sem = 0;
    int j;
    for (j = 0; j < MAX_BSM_MAPPINGS; j++)
    {
        bsm_tab[i].mappings[j].pid = -1;
        bsm_tab[i].mappings[j].vpno = 0;
        bsm_tab[i].mappings[j].npages = 0;
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int *store, int *pageth)
{
    unsigned int vpno = vaddr / NBPG;
    int i;
    for (i = 0; i < MAX_BS; i++)
    {
        bs_map_t *bs = &bsm_tab[i];
        int j;
        for (j = 0; j < MAX_BSM_MAPPINGS; j++)
        {
            if (bs->mappings[j].pid == pid)
            {
                int start_vpno = bs->mappings[j].vpno;
                int npages = bs->mappings[j].npages;
                if (vpno >= start_vpno && vpno < start_vpno + npages)
                {
                    *store = i;
                    *pageth = vpno - start_vpno;
                    return OK;
                }
            }
        }
    }
    return SYSERR; // No mapping found
}

/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    if (source < 0 || source >= MAX_BS)
    {
        return SYSERR; // Invalid backing store ID
    }
    if (bsm_tab[source].bs_private == 1 && bsm_tab[source].bs_status == BSM_MAPPED)
    {
        /* Backing store is private and already mapped */
        return SYSERR;
    }
    if (npages <= 0 || npages > 256)
    {
        return SYSERR; // Invalid number of pages
    }

    bs_map_t *bs = &bsm_tab[source];

    // Check if the process already has a mapping for this vpno
    int i;
    for (i = 0; i < MAX_BSM_MAPPINGS; i++)
    {
        if (bs->mappings[i].pid == pid && bs->mappings[i].vpno == vpno)
        {
            return SYSERR; // Mapping already exists
        }
    }

    // Find an empty mapping slot
    int mapping_index = -1;
    for (i = 0; i < MAX_BSM_MAPPINGS; i++)
    {
        if (bs->mappings[i].pid == -1)
        {
            mapping_index = i;
            break;
        }
    }
    if (mapping_index == -1)
    {
        return SYSERR; // No available mapping slots
    }

    // Add the mapping
    bs->mappings[mapping_index].pid = pid;
    bs->mappings[mapping_index].vpno = vpno;
    bs->mappings[mapping_index].npages = npages;

    // Update backing store status
    bs->bs_status = BSM_MAPPED;

    // Update bs_npages to the maximum of current bs_npages and npages
    if (bs->bs_npages < npages)
    {
        bs->bs_npages = npages;
    }

    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    int i, j;
    bs_map_t *bs = NULL;
    int mapping_index = -1;

    // Find the mapping
    for (i = 0; i < MAX_BS; i++)
    {
        bs_map_t *tmp_bs = &bsm_tab[i];
        for (j = 0; j < MAX_BSM_MAPPINGS; j++)
        {
            if (tmp_bs->mappings[j].pid == pid && tmp_bs->mappings[j].vpno == vpno)
            {
                bs = tmp_bs;
                mapping_index = j;
                break;
            }
        }
        if (mapping_index != -1)
        {
            break;
        }
    }

    if (bs == NULL || mapping_index == -1)
    {
        return SYSERR; // Mapping not found
    }

    // Remove the mapping
    bs->mappings[mapping_index].pid = -1;
    bs->mappings[mapping_index].vpno = 0;
    bs->mappings[mapping_index].npages = 0;

    // Check if there are no more mappings
    int mappings_left = 0;
    for (j = 0; j < MAX_BSM_MAPPINGS; j++)
    {
        if (bs->mappings[j].pid != -1)
        {
            mappings_left = 1;
            break;
        }
    }
    if (!mappings_left)
    {
        // No more mappings; unmap the backing store
        bs->bs_status = BSM_UNMAPPED;
        bs->bs_npages = 0;
    }
    return OK;
}
