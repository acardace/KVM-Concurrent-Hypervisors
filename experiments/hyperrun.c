#define _GNU_SOURCE
#include <argp.h>
#include <ctype.h>
#include <hyperrun.h>
#include <libgen.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* count args in -c option */
int count_core_args(char *arg) {
  int n_args = 0;
  int at_num = 0;
  int at_dash = 0;

  for (char *c = arg; *c; c++) {
    if (isdigit(*c)) {
      at_num = 1;
      /* consume the other digits */
      while (*(c + 1) && isdigit(*(c + 1))) {
        c++;
      }
      /* encountered either a slash or end of string */
      if (at_dash || !*(c + 1)) {
        n_args++;
        at_dash = at_num = 0;
      }
    } else if (*c == RANGE_DELIM && at_num) {
      at_dash = 1;
    } else if (*c == ARGS_DELIM && at_num) {
      n_args++;
    }
  }
  return (n_args > at_num) ? n_args : at_num;
}

/* parse args in -c option */
#define NUM_LEN 8
int parse_core_args(char *arg, cpu_range *cores) {
  int n_args = 0;
  int at_num = 0;
  int at_dash = 0;
  int range_min = 1000;
  int range_max = -1000;
  char num[NUM_LEN];

  memset((void *)num, 0, NUM_LEN);
  for (char *c = arg; *c; c++) {
    if (isdigit(*c)) {
      memset((void *)num, 0, NUM_LEN);
      at_num = 0;
      num[at_num] = *c;
      at_num++;
      /* consume the other digits */
      while (*(c + 1) && isdigit(*(c + 1))) {
        num[at_num] = *(c + 1);
        at_num++;
        c++;
      }
      /* encountered either a slash or end of string */
      if (at_dash) {
        range_max = atoi(num);
        if (range_max <= range_min) {
          return 0;
        }
        cores[n_args].from = range_min;
        cores[n_args].to = range_max;
        at_dash = at_num = 0;
        n_args++;
      } else if (!*(c + 1)) {
        range_max = atoi(num);
        cores[n_args].to = cores[n_args].from = range_max;
        n_args++;
      }
    } else if (*c == RANGE_DELIM && at_num) {
      range_min = atoi(num);
      at_num = 0;
      at_dash = 1;
    } else if (*c == ARGS_DELIM && at_num) {
      range_max = atoi(num);
      cores[n_args].to = cores[n_args].from = range_max;
      at_num = 0;
      n_args++;
    }
  }
  return 1;
}

/* cmd line option parser */
error_t opt_parser(int key, char *arg, struct argp_state *state) {
  int tmp;

  switch (key) {
  case 'c':
    if ((tmp = count_core_args(arg)) <= 0) {
      return ARGP_ERR_UNKNOWN;
    }
    cores_n += tmp;
    cores = (cpu_range *)realloc((void *)cores, cores_n * sizeof(cpu_range));
    if (!parse_core_args(arg, cores + (cores_n - tmp))) {
      return ARGP_ERR_UNKNOWN;
    }
    break;
  case ARGP_KEY_SUCCESS:
    /* core argument is mandatory */
    if (!cores_n) {
      argp_usage(state);
      exit(EX_USAGE);
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int cmd_index;
  cpu_set_t set;
  long max_cpu_n;
  struct argp argp = {options, opt_parser, argdoc, doc};

  argp_parse(&argp, argc, argv, ARGP_NO_ARGS, &cmd_index, 0);
  /* get the number of available cores */
  ASSERT((max_cpu_n = sysconf(_SC_NPROCESSORS_ONLN)));
  CPU_ZERO(&set);
  for (int i = 0; i < cores_n; ++i) {
    if (cores[i].from >= max_cpu_n || cores[i].to >= max_cpu_n) {
      fprintf(stderr, "%s: invalid core number\n", basename(argv[0]));
      exit(EX_DATAERR);
    }
    for (int k = cores[i].from; k <= cores[i].to; k++) {
      CPU_SET(k, &set);
    }
  }
  ASSERT(sched_setaffinity(MYPID, sizeof(cpu_set_t), &set))
  execvp(argv[cmd_index], &(argv[cmd_index]));
}
