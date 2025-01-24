/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy
 *-------------------------------------------------------------------------
 */
extern int page_replace_policy;
int debug_mode = 0; /* Initialize debugging mode to off */
/* Second-Chance Queue Variables */
int sc_queue[NFRAMES]; // Circular queue of frame numbers
int sc_head;           // Index to the head of the queue
int sc_tail;           // Index to the tail of the queue
int sc_count;          // Number of frames in the queue
void insert_frame_sc(int frame_num)
{
    /* Check if the queue is full */
    if (sc_count == NFRAMES)
    {
        kprintf("SC Queue is full. Cannot insert frame %d\n", frame_num);
        return;
    }

    sc_tail = (sc_tail + 1) % NFRAMES;
    sc_queue[sc_tail] = frame_num;
    sc_count++;
}
int get_frame_sc(void)
{
    while (TRUE)
    {
        if (sc_count == 0)
        {
            kprintf("SC Queue is empty. No frame to replace.\n");
            return SYSERR;
        }

        int frame_num = sc_queue[sc_head];

        // Get the page table entry for this frame
        int pid = frm_tab[frame_num].fr_pid;
        unsigned int vpno = frm_tab[frame_num].fr_vpno;
        unsigned long vaddr = vpno * NBPG;
        unsigned int pde_index = (vaddr >> 22) & 0x3FF;
        unsigned int pte_index = (vaddr >> 12) & 0x3FF;

        pd_t *pd = (pd_t *)(proctab[pid].pdbr);
        if (pd[pde_index].pd_pres == 0)
        {
            // Page directory entry not present, skip this frame
            sc_head = (sc_head + 1) % NFRAMES;
            continue;
        }

        pt_t *pt = (pt_t *)(pd[pde_index].pd_base * NBPG);
        if (pt[pte_index].pt_pres == 0)
        {
            // Page table entry not present, skip this frame
            sc_head = (sc_head + 1) % NFRAMES;
            continue;
        }

        // Check the reference bit (pt_acc)
        if (pt[pte_index].pt_acc == 0)
        {
            // Reference bit is 0; select this frame

            // Remove the frame from the queue
            sc_queue[sc_head] = -1;
            sc_head = (sc_head + 1) % NFRAMES;
            sc_count--;

            // Print the frame number if debugging mode is enabled
            if (debug_mode)
            {
                kprintf("Printing the frame number in debug mode: %d\n", frame_num);
            }

            return frame_num;
        }
        else
        {
            // Reference bit is 1; give a second chance
            pt[pte_index].pt_acc = 0;

            // Move sc_head to next frame
            sc_head = (sc_head + 1) % NFRAMES;
        }
    }
}
void replace_frame_sc(int frame_num)
{
    // Get information about the frame
    int pid = frm_tab[frame_num].fr_pid;
    unsigned int vpno = frm_tab[frame_num].fr_vpno;
    unsigned long vaddr = vpno * NBPG;
    unsigned int pde_index = (vaddr >> 22) & 0x3FF;
    unsigned int pte_index = (vaddr >> 12) & 0x3FF;

    pd_t *pd = (pd_t *)(proctab[pid].pdbr);
    pt_t *pt = (pt_t *)(pd[pde_index].pd_base * NBPG);

    // Handle dirty page
    if (pt[pte_index].pt_dirty == 1)
    {
        // Write back to backing store
        int store, pageth;
        if (bsm_lookup(pid, vaddr, &store, &pageth) == SYSERR)
        {
            kprintf("Frame Replacement: Invalid mapping\n");
            kill(pid);
            return;
        }
        char *src = (char *)((frame_num + FRAME0) * NBPG);
        write_bs(src, store, pageth);
    }

    // Update page table entry
    pt[pte_index].pt_pres = 0;

    // Invalidate TLB entry if necessary
    if (pid == currpid)
    {
        invalidate_tlb(vaddr);
    }

    // Update frame table
    frm_tab[frame_num].fr_status = FRM_UNMAPPED;
    frm_tab[frame_num].fr_pid = -1;
    frm_tab[frame_num].fr_vpno = 0;
    frm_tab[frame_num].fr_refcnt = 0;
    frm_tab[frame_num].fr_type = FR_PAGE;
    frm_tab[frame_num].fr_dirty = 0;
}
SYSCALL srpolicy(int policy)
{
    if (policy == SC)
    {
        debug_mode = 1;
        page_replace_policy = SC;
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
    return page_replace_policy;
}
