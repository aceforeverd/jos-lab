#include <kern/e1000.h>
#include <inc/memlayout.h>
#include <lib/syscall.c>
#include <inc/assert.h>

#define E1000_START (KSTACKTOP + PGSIZE)
#define E1000_SIZE (1 << 22)

#define E1000_DEBUG 1

// LAB 6: Your driver code here

int e1000_attach(struct pci_func *pcif) {
#ifdef E1000_DEBUG
    cprintf("e1000_attach: \n");
    cprintf("reg_base[0]: %8x, reg_size[0]: %8x\n", pcif->reg_base[0], pcif->reg_size[0]);
#endif
    pcif->reg_base[0] = E1000_START;
    pcif->reg_size[0] = E1000_SIZE;

    uintptr_t addr_base = pcif->reg_base[0];
    int r;
    for (uintptr_t off = 0; off < pcif->reg_size[0]; off += PGSIZE) {
        if ((r = sys_page_alloc(0, (void *) (addr_base + off), PTE_PCD | PTE_PWT)) < 0) {
            panic("sys_page_alloc: %e", r);
        }
    }
    pci_func_enable(pcif);
    return 0;
}
