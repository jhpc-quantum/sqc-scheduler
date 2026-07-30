#include "sqc_rpc_perf_client.h"


//
// Print the help message.
//
static inline void
s_print_help(void) {
  printf("Usage: %s SUB-COMMAND [ARG...]\n", program_name);
  printf("       %s --help\n", program_name);
  printf("\n");
  printf("Sub-commands:\n");
  printf("  submit                 submit a job\n");
  printf("\n");
  printf("Try '%s SUB-COMMAND --help' for more details\n", program_name);
}


//
// Main.
//
int
main(int argc, char *argv[]) {
  int arg_index = 1;
  char *sub_command = NULL;

  while (arg_index < argc) {
    char *arg = argv[arg_index];
    if (arg[0] == '-') {
      if (strcmp(arg, "--") == 0) {
        arg_index += 1;
        break;
      } else if (strcmp(arg, "--help") == 0) {
        s_print_help();
        return 0;
      } else if (arg[1] == '\0') {
        break;
      } else {
        fprintf(stderr, "invalid option '%s'\n", arg);
        return 1;
      }
    } else {
      break;
    }
    arg_index++;
  }

  if (unlikely(arg_index == argc)) {
    fprintf(stderr, "missing sub-command\n");
    return 1;
  }

  sub_command = argv[arg_index++];
  if (strcmp(sub_command, "submit") == 0) {
    return (subcmd_submit_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else {
    fprintf(stderr, "invalid sub-command: %s\n", sub_command);
    return 1;
  }
}
