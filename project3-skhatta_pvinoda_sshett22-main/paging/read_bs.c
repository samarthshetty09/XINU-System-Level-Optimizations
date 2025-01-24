#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {
    /// Validate inputs
    if (bs_id < 0 || bs_id >= MAX_BS) {
        return SYSERR;
    }

    if (page < 0 || page >= 256) { // Assuming 256 pages per backing store
        return SYSERR;
    }

    bs_map_t *bs_entry = &bsm_tab[bs_id];

    if (bs_entry->bs_status == BSM_UNMAPPED) {
        return SYSERR; // Backing store is not in use
    }

    // Check if the current process has a mapping to this backing store
    int pid = currpid;
    int mapping_found = 0;
    int i;
    for (i = 0; i < MAX_BSM_MAPPINGS; i++) {
        if (bs_entry->mappings[i].pid == pid) {
            // Optionally, check if the page number is within the allocated range
            int npages = bs_entry->mappings[i].npages;
            if (page < npages) {
                mapping_found = 1;
                break;
            } else {
                return SYSERR; // Page number out of bounds for this mapping
            }
        }
    }

    if (!mapping_found) {
        return SYSERR; // Current process does not have a mapping to this backing store
    }

    // Calculate physical address
    void *phy_addr = (void *)(BACKING_STORE_BASE + bs_id * BACKING_STORE_UNIT_SIZE + page * NBPG);

    // Read from backing store into destination
    bcopy(phy_addr, (void *)dst, NBPG);

    return OK;
}
