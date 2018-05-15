// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

static int has_entry(uintptr_t addr) {
    return (vpd[PDX(addr)] & PTE_P) && (vpt[PGNUM(addr)] & PTE_P);
}

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) ROUNDDOWN(utf->utf_fault_va, PGSIZE);
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

    if (!(err & FEC_WR)) {
        panic("error code do not have FEC_WR\n");
    }

    if (!(vpd[PDX(addr)] & PTE_P) || !(vpt[PGNUM(addr)] & PTE_COW) || !(vpt[PGNUM(addr)] & PTE_P)) {
        panic("%xis not a copy on write page", addr);
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
    r = sys_page_alloc(0, PFTEMP,  PTE_P | PTE_U | PTE_W);
    if (r < 0) {
        panic("pgfault.sys_page_alloc: %e", r);
    }
    memmove(PFTEMP, addr, PGSIZE);

    if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_W | PTE_U)) < 0) {
        panic("pgfault.sys_page_map: %e", r);
    }

    if ((r = sys_page_unmap(0, PFTEMP)) < 0) {
        panic("pgfault.sys_page_unmap: %e", r);
    }
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
    int perm;

	// LAB 4: Your code here.
    uintptr_t addr = pn * PGSIZE;
    perm = PTE_P | PTE_U;
    if (!has_entry(addr)) {
        /* return silently */
        return 0;
    }

    if ((vpt[pn] & PTE_W) || (vpt[pn] & PTE_COW) ) {
        perm |= PTE_COW;
    }

    if ((r = sys_page_map(0, (void *)addr, envid, (void *)addr, perm)) < 0) {
        return r;
    }

    if (!(perm & PTE_COW)) {
        return 0;
    }

    if ((r = sys_page_map(0, (void *)addr, 0, (void *)addr, perm)) < 0) {
        return r;
    }

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
    set_pgfault_handler(pgfault);

    envid_t child = sys_exofork();
    if (child < 0) {
        return child;
    }

    if (child == 0) {
        /* child */
        thisenv = envs + ENVX(sys_getenvid());
        return 0;
    }

    int r;
    for (uintptr_t addr = 0; addr < UTOP - PGSIZE; addr += PGSIZE) {
        if ((r = duppage(child, addr / PGSIZE)) < 0) {
            panic("fork.duppage: %e", r);
        }
    }

    /* exception stack */
    if ((r = sys_page_alloc(child, (void *) (UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P )) < 0) {
        panic("exception stack allocation failed: %e", r);
    }

    extern void _pgfault_upcall(void);
    if ((r = sys_env_set_pgfault_upcall(child, _pgfault_upcall)) < 0) {
        panic("set pgfault upcall: %e", r);
    }

    if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0) {
        panic("set child runnable: %e", r);
    }
    return child;
}

// Challenge!
int
sfork(void)
{
    int r;

    set_pgfault_handler(pgfault);

    thisenv = NULL;

    envid_t child = sys_exofork();
    if (child < 0)
        panic("sys_exofork: %e", child);
    if (child == 0) {
        thisenv = envs + ENVX(sys_getenvid());
        return 0;
    }

    uintptr_t addr;
    for (addr = 0; addr < UTOP; addr += PGSIZE) {
        if (addr != UXSTACKTOP - PGSIZE && addr != USTACKTOP - PGSIZE) {
            if (has_entry(addr)) {
                if ((r = sys_page_map(0, (void *)addr, child, (void *)addr, vpt[PGNUM(addr)])) < 0) {
                    panic("sfork.sys_page_map at addr %08x: %e", addr, r);
                }
            }
        }
    }

    /* exception stack: standalone */
    if ((r = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P)) < 0) {
        panic("sfork: exception stack allocation failed: %e", r);
    }

    /* user stack: copy on write */
    if ((r = duppage(child, USTACKTOP / PGSIZE - 1)) < 0) {
        panic("sfork: failed to duppage: %e", r);
    }

    extern void _pgfault_upcall(void);
    if ((r = sys_env_set_pgfault_upcall(child, _pgfault_upcall)) < 0) {
        panic("sfork: set_pgfault_upcall: %e", r);
    }

    if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
        panic("sfork.sys_env_set_status: %e", r);

    return child;
}
