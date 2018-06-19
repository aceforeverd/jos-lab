#include <kern/e1000.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <kern/pmap.h>

volatile char *E1000_START = (char *)(KSTACKTOP + PGSIZE);
#define E1000_SIZE (1 << 22)

#define E1000_DEBUG 1

// LAB 6: Your driver code here

int e1000_attach(struct pci_func *pcif) {
    pci_func_enable(pcif);
#ifdef E1000_DEBUG
    cprintf("e1000_attach: \n");
    cprintf("reg_base[0]: %8x, reg_size[0]: %8x\n", pcif->reg_base[0], pcif->reg_size[0]);
#endif
    boot_map_region(kern_pgdir, E1000_START, pcif->reg_size[0], pcif->reg_base[0], PTE_PCD | PTE_PWT);

    return 0;
}
