#ifndef __HYPERRUN_H__
#define __HYPERRUN_H__
#include <sysexits.h>

#define MYPID 0
#define ASSERT(x)                                                              \
  if (x == -1) {                                                               \
    perror("hyperrun");                                                        \
    exit(EX_UNAVAILABLE);                                                      \
  }

#define RANGE_DELIM '-'
#define ARGS_DELIM ','

/* global vars for cmd line arguments */
typedef struct cpu_range { unsigned int from, to; } cpu_range;
cpu_range *cores = NULL;
unsigned int cores_n = 0;

/* variable for argp (command line parsing) */
const char *argp_program_version = "hyperrun 0.1";
const char *argp_program_bug_address = "<antonio@cardace.it>";
static char doc[] =
    "Hyperrun -- Run two or more different Hypervisor concurrently with "
    "Intel VT-x hardware-assisted virtualization enabled"
    "\vThe "
    "CORE argument is mandatory and HYPERVISOR-CMD can be any "
    "command which starts a hypervisor such as kvm (Qemu) or "
    "VirtualBox.";
static char argdoc[] = "HYPERVISOR-CMD";

static struct argp_option options[] = {
    {"core", 'c', "CORE", 0, "cores on which the Hypervisor will run"}, {0}};

#endif
