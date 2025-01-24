// paging.c
#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <control_reg.h>
#include <frame.h>
#include <mem.h>

unsigned long global_pdbr; // Global Page Directory Base Register
int global_pt_frames[4];   // Store frame numbers of global page tables

/* Function to initialize paging */
SYSCALL init_paging()
{
    int pd_frame_num;
    int pt_frame_num;
    int i, j;
    pd_t *page_dir;
    pt_t *page_tab;

    /* Allocate a frame for the page directory */
    if (get_frm(&pd_frame_num) == SYSERR)
    {
        kprintf("init_paging: Unable to allocate frame for page directory\n");
        return SYSERR;
    }

    /* Update frame table entry */
    frm_tab[pd_frame_num].fr_status = FRM_MAPPED;
    frm_tab[pd_frame_num].fr_pid = NULLPROC;
    frm_tab[pd_frame_num].fr_type = FR_DIR;
    frm_tab[pd_frame_num].fr_refcnt = 4; // number of page tables mapped

    /* Set the PDBR for the null process */
    proctab[NULLPROC].pdbr = (pd_frame_num + FRAME0) * NBPG;

    /* Initialize the page directory */
    page_dir = (pd_t *)((pd_frame_num + FRAME0) * NBPG);
    for (i = 0; i < NENTRIES; i++)
    {
        page_dir[i].pd_pres = 0;
        page_dir[i].pd_write = 1;
        page_dir[i].pd_user = 0;
        page_dir[i].pd_pwt = 0;
        page_dir[i].pd_pcd = 0;
        page_dir[i].pd_acc = 0;
        page_dir[i].pd_mbz = 0;
        page_dir[i].pd_fmb = 0;
        page_dir[i].pd_global = 0;
        page_dir[i].pd_avail = 0;
        page_dir[i].pd_base = 0;
    }

    /* Allocate and initialize the first 4 page tables */
    for (i = 0; i < 4; i++)
    {
        /* Allocate a frame for the page table */
        if (get_frm(&pt_frame_num) == SYSERR)
        {
            kprintf("init_paging: Unable to allocate frame for page table %d\n", i);
            return SYSERR;
        }

        /* Store the frame number */
        global_pt_frames[i] = pt_frame_num;

        /* Update frame table entry */
        frm_tab[pt_frame_num].fr_status = FRM_MAPPED;
        frm_tab[pt_frame_num].fr_pid = NULLPROC;
        frm_tab[pt_frame_num].fr_type = FR_TBL;
        frm_tab[pt_frame_num].fr_refcnt = NENTRIES; // Number of entries in page table

        /* Set up the page directory entry */
        page_dir[i].pd_pres = 1;
        page_dir[i].pd_write = 1;
        page_dir[i].pd_user = 0;
        page_dir[i].pd_pwt = 0;
        page_dir[i].pd_pcd = 0;
        page_dir[i].pd_acc = 0;
        page_dir[i].pd_mbz = 0;
        page_dir[i].pd_fmb = 0;
        page_dir[i].pd_global = 1; // Mark as global
        page_dir[i].pd_avail = 0;
        page_dir[i].pd_base = pt_frame_num + FRAME0;

        /* Initialize the page table */
        page_tab = (pt_t *)((pt_frame_num + FRAME0) * NBPG);
        for (j = 0; j < NENTRIES; j++)
        {
            page_tab[j].pt_pres = 1;
            page_tab[j].pt_write = 1;
            page_tab[j].pt_user = 0;
            page_tab[j].pt_pwt = 0;
            page_tab[j].pt_pcd = 0;
            page_tab[j].pt_acc = 0;
            page_tab[j].pt_dirty = 0;
            page_tab[j].pt_mbz = 0;
            page_tab[j].pt_global = 1; 
            page_tab[j].pt_avail = 0;
            page_tab[j].pt_base = (i * NENTRIES) + j;
        }
    }

    /* Set CR3 to point to the page directory of the null process */
    write_cr3(proctab[NULLPROC].pdbr);

    /* Install the page fault handler */
    set_evec(14, (u_long)pfintr);

    /* Enable paging */
    enable_paging();

    return OK;
}

/* Initialize the page directory for a process */
int initialize_page_directory(int pid)
{
    int frame_num;
    int i;
    pd_t *page_dir;

    /* Allocate a frame for the page directory */
    if (get_frm(&frame_num) == SYSERR)
    {
        kprintf("initialize_page_directory: Unable to allocate frame for page directory\n");
        return SYSERR;
    }

    /* Update frame table entry */
    frm_tab[frame_num].fr_status = FRM_MAPPED;
    frm_tab[frame_num].fr_pid = pid;
    frm_tab[frame_num].fr_type = FR_DIR;
    frm_tab[frame_num].fr_refcnt = 4; 

    /* Set the PDBR for the process */
    proctab[pid].pdbr = (frame_num + FRAME0) * NBPG;

    /* Initialize the page directory */
    page_dir = (pd_t *)((frame_num + FRAME0) * NBPG);
    for (i = 0; i < NENTRIES; i++)
    {
        page_dir[i].pd_pres = 0;
        page_dir[i].pd_write = 1;
        page_dir[i].pd_user = 0;
        page_dir[i].pd_pwt = 0;
        page_dir[i].pd_pcd = 0;
        page_dir[i].pd_acc = 0;
        page_dir[i].pd_mbz = 0;
        page_dir[i].pd_fmb = 0;
        page_dir[i].pd_global = 0;
        page_dir[i].pd_avail = 0;
        page_dir[i].pd_base = 0;
    }

    /* Map the global page tables */
    for (i = 0; i < 4; i++)
    {
        page_dir[i].pd_pres = 1;
        page_dir[i].pd_write = 1;
        page_dir[i].pd_user = 0;
        page_dir[i].pd_pwt = 0;
        page_dir[i].pd_pcd = 0;
        page_dir[i].pd_acc = 0;
        page_dir[i].pd_mbz = 0;
        page_dir[i].pd_fmb = 0;
        page_dir[i].pd_global = 1; 
        page_dir[i].pd_avail = 0;
        page_dir[i].pd_base = global_pt_frames[i] + FRAME0;
    }

    return OK;
}

/* Create a page table for a process at a given page directory entry index */
int create_page_table(int pid, unsigned int pde_index)
{
    int frame_num;
    pd_t *pd = (pd_t *)(proctab[pid].pdbr);

    // Allocate a frame for the page table
    if (get_frm(&frame_num) == SYSERR)
    {
        kprintf("create_page_table: No free frames for page table\n");
        return SYSERR;
    }

    frm_tab[frame_num].fr_type = FR_TBL;
    frm_tab[frame_num].fr_status = FRM_MAPPED;
    frm_tab[frame_num].fr_pid = pid;
    frm_tab[frame_num].fr_refcnt = 0; // Initially zero

    pt_t *pt = (pt_t *)((frame_num + FRAME0) * NBPG);
    int i;
    for (i = 0; i < NENTRIES; i++)
    {
        pt[i].pt_pres = 0;
        pt[i].pt_write = 1;
        pt[i].pt_user = 0;
        pt[i].pt_pwt = 0;
        pt[i].pt_pcd = 0;
        pt[i].pt_acc = 0;
        pt[i].pt_dirty = 0;
        pt[i].pt_mbz = 0;
        pt[i].pt_global = 0;
        pt[i].pt_avail = 0;
        pt[i].pt_base = 0;
    }

    // Update the page directory entry
    pd[pde_index].pd_pres = 1;
    pd[pde_index].pd_write = 1;
    pd[pde_index].pd_user = 0;
    pd[pde_index].pd_pwt = 0;
    pd[pde_index].pd_pcd = 0;
    pd[pde_index].pd_acc = 0;
    pd[pde_index].pd_mbz = 0;
    pd[pde_index].pd_fmb = 0;
    pd[pde_index].pd_global = 0;
    pd[pde_index].pd_avail = 0;
    pd[pde_index].pd_base = frame_num + FRAME0;

    return OK;
}

/* Free paging resources used by the process */
void free_paging_resources(int pid)
{
    int i;

    /* Unmap any backing stores mapped by the process */
    for (i = 0; i < MAX_BS; i++)
    {
        bsm_unmap(pid, proctab[pid].vhpno, 0);
    }

    /* Free frames used by the process's page tables and page directory */
    for (i = 0; i < NFRAMES; i++)
    {
        if (frm_tab[i].fr_pid == pid)
        {
            free_frm(i);
        }
    }
}

/* Invalidate TLB entry for a given virtual address */
void invalidate_tlb(unsigned long vaddr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}
