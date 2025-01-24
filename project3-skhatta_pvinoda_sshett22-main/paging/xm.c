/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
    kprintf("inside xmmap now\n");

    STATWORD ps;
    disable(ps);

    int pid = currpid;

    // inpjut validation
    if (virtpage < 4096)
    {
        restore(ps);
        return SYSERR;
    }

    if (npages <= 0 || npages > 256)
    {
        restore(ps);
        return SYSERR;
    }

    if (source < 0 || source >= MAX_BS)
    {
        restore(ps);
        return SYSERR; 
    }

    if (bsm_map(pid, virtpage, source, npages) == SYSERR)
    {
        restore(ps);
        return SYSERR;
    }

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    disable(ps);

    int pid = currpid;

    if (virtpage < 4096)
    {
        restore(ps);
        return SYSERR;
    }

    //  unmapping the backing store
    if (bsm_unmap(pid, virtpage, 0) == SYSERR)
    {
        restore(ps);
        return SYSERR;
    }

    // cleaning up the page tables and frames
    int bs_id = -1;
    int npages = 0;
    int i;
    for (i = 0; i < MAX_BS; i++)
    {
        bs_map_t *bs = &bsm_tab[i];
        int j;
        for (j = 0; j < MAX_BSM_MAPPINGS; j++)
        {
            if (bs->mappings[j].pid == pid && bs->mappings[j].vpno == virtpage)
            {
                bs_id = i;
                npages = bs->mappings[j].npages;
                break;
            }
        }
        if (bs_id != -1)
        {
            break;
        }
    }

    if (bs_id == -1 || npages == 0)
    {
        restore(ps);
        return SYSERR;
    }

    for (i = 0; i < npages; i++)
    {
        int vpno = virtpage + i;
        unsigned long vaddr = vpno * NBPG;
        unsigned int pde_index = (vaddr >> 22) & 0x3FF;
        unsigned int pte_index = (vaddr >> 12) & 0x3FF;

        pd_t *pd = (pd_t *)(proctab[pid].pdbr);
        if (pd[pde_index].pd_pres == 0)
        {
            continue; 
        }

        pt_t *pt = (pt_t *)(pd[pde_index].pd_base * NBPG);
        if (pt[pte_index].pt_pres == 0)
        {
            continue; 
        }

        int frame_num = pt[pte_index].pt_base - FRAME0;

        // handling the dirty page
        if (pt[pte_index].pt_dirty == 1)
        {
            char *src = (char *)(vaddr);
            write_bs(src, bs_id, vpno - virtpage);
        }

        free_frm(frame_num);

        pt[pte_index].pt_pres = 0;
        pt[pte_index].pt_write = 0;
        pt[pte_index].pt_user = 0;
        pt[pte_index].pt_pwt = 0;
        pt[pte_index].pt_pcd = 0;
        pt[pte_index].pt_acc = 0;
        pt[pte_index].pt_dirty = 0;
        pt[pte_index].pt_mbz = 0;
        pt[pte_index].pt_global = 0;
        pt[pte_index].pt_avail = 0;
        pt[pte_index].pt_base = 0;

        invalidate_tlb(vaddr);

        int pt_frame_num = pd[pde_index].pd_base - FRAME0;
        frm_tab[pt_frame_num].fr_refcnt--;
        if (frm_tab[pt_frame_num].fr_refcnt == 0)
        {
            free_frm(pt_frame_num);

            // now, update the page directory
            pd[pde_index].pd_pres = 0;
            pd[pde_index].pd_write = 0;
            pd[pde_index].pd_user = 0;
            pd[pde_index].pd_pwt = 0;
            pd[pde_index].pd_pcd = 0;
            pd[pde_index].pd_acc = 0;
            pd[pde_index].pd_mbz = 0;
            pd[pde_index].pd_fmb = 0;
            pd[pde_index].pd_global = 0;
            pd[pde_index].pd_avail = 0;
            pd[pde_index].pd_base = 0;
        }
    }

    restore(ps);
    return OK;
}
