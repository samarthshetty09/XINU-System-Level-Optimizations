/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
/* Define the frame table */
fr_map_t frm_tab[NFRAMES];
SYSCALL init_frm()
{
    int i;
    for (i = 0; i < NFRAMES; i++)
    {
        frm_tab[i].fr_status = FRM_UNMAPPED;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_vpno = 0;
        frm_tab[i].fr_refcnt = 0;
        frm_tab[i].fr_type = FR_PAGE;
        frm_tab[i].fr_dirty = 0;
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int *avail)
{
    STATWORD ps;
    disable(ps);

    // Search for a free frame
    int i;
    for (i = 0; i < NFRAMES; i++)
    {
        if (frm_tab[i].fr_status == FRM_UNMAPPED)
        {
            *avail = i;
            restore(ps);
            return OK;
        }
    }

    // No free frames; invoke page replacement policy
    int frame_to_replace = -1;
    if (grpolicy() == SC)
    {
        frame_to_replace = get_frame_sc();
    }
    else
    {
        // Implement other policies if necessary
        restore(ps);
        return SYSERR;
    }

    if (frame_to_replace == -1)
    {
        // No frame could be replaced
        restore(ps);
        return SYSERR;
    }

    // Replace the frame
    *avail = frame_to_replace;
    replace_frame_sc(frame_to_replace);

    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

    if (i < 0 || i >= NFRAMES)
    {
        return SYSERR;
    }

    STATWORD ps;
    disable(ps);

    // Reset frame table entry
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid = -1;
    frm_tab[i].fr_vpno = 0;
    frm_tab[i].fr_refcnt = 0;
    frm_tab[i].fr_type = FR_PAGE;
    frm_tab[i].fr_dirty = 0;

    restore(ps);
    return OK;
}
