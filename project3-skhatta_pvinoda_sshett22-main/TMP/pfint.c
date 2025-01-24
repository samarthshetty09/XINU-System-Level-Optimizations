/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern unsigned long pferrcode; // Declared in pfintr.S

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
    STATWORD ps;
    disable(ps);
    // kprintf("From page fault interrupt");
    unsigned long faulted_address;
    int pid = currpid;
    int store, pageth;

    // Get the faulted address from CR2 register
    faulted_address = read_cr2();

    // Calculate the virtual page number (vpno)
    unsigned int vpno = faulted_address / NBPG;

    // Validate the faulted address
    if (vpno < 4096)
    {
        // Accessing invalid memory (global memory or unmapped region)
        kprintf("Page Fault: Invalid access by process %d at address 0x%08X\n", pid, faulted_address);
        kill(pid);
        restore(ps);
        return SYSERR;
    }

    // Use bsm_lookup to find the backing store
    if (bsm_lookup(pid, faulted_address, &store, &pageth) == SYSERR)
    {
        kprintf("Page Fault: No backing store mapping for address 0x%08X\n", faulted_address);
        kill(pid);
        restore(ps);
        return SYSERR;
    }

    // Get the page directory and page table indices
    unsigned int pde_index = (faulted_address >> 22) & 0x3FF;
    unsigned int pte_index = (faulted_address >> 12) & 0x3FF;

    // Get the page directory
    pd_t *pd = (pd_t *)(proctab[pid].pdbr);

    // Check if the page table exists
    if (pd[pde_index].pd_pres == 0)
    {
        // Create a page table
        if (create_page_table(pid, pde_index) == SYSERR)
        {
            kprintf("pfint: Failed to create page table\n");
            kill(pid);
            restore(ps);
            return SYSERR;
        }
    }

    // Get the page table
    pt_t *pt = (pt_t *)(pd[pde_index].pd_base * NBPG);

    // Check if the page is present
    if (pt[pte_index].pt_pres == 0)
    {
        // Allocate a frame for the page
        int page_frame;
        if (get_frm(&page_frame) == SYSERR)
        {
            kprintf("pfint: No free frames for page\n");
            kill(pid);
            restore(ps);
            return SYSERR;
        }

        // Read the page from backing store
        char *dst = (char *)((page_frame + FRAME0) * NBPG);
        if (read_bs(dst, store, pageth) == SYSERR)
        {
            kprintf("pfint: Failed to read from backing store\n");
            free_frm(page_frame);
            kill(pid);
            restore(ps);
            return SYSERR;
        }

        // Update the page table entry
        pt[pte_index].pt_pres = 1;
        pt[pte_index].pt_write = 1;
        pt[pte_index].pt_user = 0;
        pt[pte_index].pt_pwt = 0;
        pt[pte_index].pt_pcd = 0;
        pt[pte_index].pt_acc = 0;
        pt[pte_index].pt_dirty = 0;
        pt[pte_index].pt_mbz = 0;
        pt[pte_index].pt_global = 0;
        pt[pte_index].pt_avail = 0;
        pt[pte_index].pt_base = page_frame + FRAME0;

        // Update frame table
        frm_tab[page_frame].fr_status = FRM_MAPPED;
        frm_tab[page_frame].fr_pid = pid;
        frm_tab[page_frame].fr_vpno = vpno;
        frm_tab[page_frame].fr_refcnt = 1;
        frm_tab[page_frame].fr_type = FR_PAGE;
        frm_tab[page_frame].fr_dirty = 0;

        // Increment reference count for page table frame
        int pt_frame_num = pd[pde_index].pd_base - FRAME0;
        frm_tab[pt_frame_num].fr_refcnt++;

        // Insert into SC queue if using Second-Chance replacement
        if (grpolicy() == SC)
        {
            insert_frame_sc(page_frame);
        }
    }

    // Invalidate TLB entry
    invalidate_tlb(faulted_address);

    restore(ps);
    return OK;
}
