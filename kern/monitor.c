// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
typedef struct mapping_cmd_ {
    enum {
        HELP,
        DUMP,
        UNKNOWN,
    } type;
    uintptr_t start;
    uintptr_t end;
} mapping_cmd;

static void mapping_parse(int argv, char **argc, mapping_cmd *cmd);
static void mapping_help();
static void mapping_execute(mapping_cmd *cmd);
static void mapping_dump(uintptr_t start, uintptr_t end);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display the stack backtrace", mon_backtrace },
    { "time", "Count the time (cpu cycles) of a command", mon_time },
    { "mapping", "util to manipulate physical address mappings", mon_mapping },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

int mon_time(int argc, char **argv, struct Trapframe *tf) {
    if (argc <= 1) {
        cprintf("No command to count\n");
        cprintf("Usage:\n");
        cprintf("\ttime [command] [command arguments ...]\n\n");
        return -1;
    }

    char *cmd = argv[1];
    int found = 0;
    for (int i = 0; i < NCOMMANDS; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            found = 1;
            uint32_t start_time_high, start_time_low, end_time_low, end_time_high;
            uint64_t start, end;
            __asm __volatile (
                    "rdtsc\n\t"
                    "movl %%edx, %0\n\t"
                    "movl %%eax, %1\n\t"
                    : "=r" (start_time_high), "=r" (start_time_low)
                    : : "%eax", "%edx"
                    );

            commands[i].func(argc - 1, argv + 1, tf);

            __asm __volatile (
                    "rdtsc\n\t"
                    "movl %%edx, %0\n\t"
                    "movl %%eax, %1\n\t"
                    : "=r" (end_time_high), "=r" (end_time_low)
                    : : "%eax", "%edx"
                    );

            start = ((uint64_t)start_time_high << 32) | start_time_low;
            end = ((uint64_t)end_time_high << 32) | end_time_low;
            cprintf("%s cycles: %ld\n", cmd, end - start);
            return 0;
        }
    }
    if (found == 0) {
        cprintf("command %s not found\n", cmd);
        return -1;
    }

    cprintf("unexpected error\n");
    return -1;
}

int mon_mapping(int argc, char **argv, struct Trapframe *tf) {
    mapping_cmd cmd;
    mapping_parse(argv, argc, &cmd);
    mapping_execute(&cmd);
    return 0;
}

static void mapping_parse(int argv, char **argc, mapping_cmd *cmd) {
    assert(cmd);
    if (argv <= 1) {
        cprintf("option required\n");
        mapping_help();
        return;
    }
    char *option = argc[1];
    if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
        cmd->type = HELP;
        return;
    }
    if ((strcmp(option, "-s") == 0) || strcmp(option, "--show") == 0) {
        cmd->type = DUMP;
        cmd->start = 0x0;
        cmd->end = ~0x0;
        if (argv == 3) {
            cmd->start = atoi(argc[2]);
        } else if (argv >= 4) {
            cmd->start = atoi(argc[2]);
            cmd->end = atoi(argc[3]);
        }
    } else {
        cmd->type = UNKNOWN;
    }
}

static void mapping_help() {
    cprintf("mapping [option] [start_addr] [end_addr]\n");
    cprintf("options: \n");
    cprintf("\t-h | --help: show this help info\n");
    cprintf("\t-s | --show: dump mapping info from start_addr to end_addr\n");
}

static void mapping_execute(mapping_cmd *cmd) {
    switch (cmd->type) {
        case HELP:
            mapping_help();
            break;
        case DUMP:
            mapping_dump(cmd->start, cmd->end);
            break;
        default:
            cprintf("unknown option\n");
            mapping_help();
    }
}

static void mapping_dump(uintptr_t start, uintptr_t end) {

}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr)); 
    return pretaddr;
}

static uint32_t read_ret_pointer(uint32_t ebp)
{
    uint32_t ret_pointer;
    __asm __volatile(
            "movl (%1), %0"
            : "=r" (ret_pointer)
            : "r" (ebp + 4)
            );
    return ret_pointer;
}

static uint32_t
read_prev_ebp(uint32_t curr_ebp)
{
    uint32_t prev_ebp;
    __asm __volatile(
            "movl (%1), %0"
            : "=r" (prev_ebp)
            : "r" (curr_ebp)
            );
    return prev_ebp;
}

static uint32_t
read_parameter(uint32_t ebp, uint32_t offset)
{
    uint32_t param;
    __asm __volatile(
            "movl (%1), %0"
            : "=r" (param)
            : "r" (ebp + offset)
            );
    return param;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    cprintf("Stack backtrace:\n");
    uint32_t ebp = read_ebp();
    uint32_t eip = read_eip();
    while (ebp != 0x0) {
        struct Eipdebuginfo info = {
            "unknow", 0, "unknow", 7, 0, 0
        };
        int result = debuginfo_eip(eip, &info);

        cprintf("eip %08x ebp %08x args %08x %08x %08x %08x %08x\n",
                eip, ebp,
                read_parameter(ebp, 8), read_parameter(ebp, 12), read_parameter(ebp, 16),
                read_parameter(ebp, 20), read_parameter(ebp, 24)
               );
        cprintf("%s:%d: %s+%d\n", info.eip_file, info.eip_line,
                info.eip_fn_name, eip - info.eip_fn_addr);
        ebp = read_prev_ebp(ebp);
        // get the prev eip
        eip = read_ret_pointer(ebp) - 4;
    }
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
