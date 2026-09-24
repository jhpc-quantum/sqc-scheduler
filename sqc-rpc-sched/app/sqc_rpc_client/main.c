#include "sqc_rpc_client.h"


//
// Print the help message.
//
static inline void
s_print_help(void) {
  printf("Usage: %s SUB-COMMAND [ARG...]\n", program_name);
  printf("       %s --help\n", program_name);
  printf("\n");
  printf("Sub-commands:\n");
  printf("  submit                         submit a job\n");
  printf("  status                         get status of the submitted job\n");
  printf("  cancel                         cancel the submitted job\n");
  printf("  delete                         delete the submitted job\n");
  printf("  list                           list submitted jobs\n");
  printf("  adm-del-jobs                   delete submitted jobs\n");
  printf("  adm-add-user                   add a user\n");
  printf("  adm-set-user-status            set user status\n");
  printf("  adm-set-group-exec-time-limit  set group executable time limit\n");
  printf("  adm-set-user-group-status      set user-group association status\n");
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
  } else if (strcmp(sub_command, "status") == 0) {
    return (subcmd_status_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "cancel") == 0) {
    return (subcmd_cancel_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "delete") == 0) {
    return (subcmd_delete_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "list") == 0) {
    return (subcmd_list_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "adm-del-jobs") == 0) {
    return (subcmd_adm_del_jobs_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "adm-add-user") == 0) {
    return (subcmd_adm_add_user_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "adm-set-user-status") == 0) {
    return (subcmd_adm_set_user_status_main(argc, argv, arg_index) == SQC_RESULT_OK) ? 0 : 1;
  } else if (strcmp(sub_command, "adm-set-group-exec-time-limit") == 0) {
    return subcmd_adm_set_group_exec_time_limit_main(argc, argv, arg_index) == SQC_RESULT_OK ? 0 : 1;
  } else if (strcmp(sub_command, "adm-set-user-group-status") == 0) {
    return subcmd_adm_set_user_group_status_main(argc, argv, arg_index) == SQC_RESULT_OK ? 0 : 1;
  } else {
    fprintf(stderr, "invalid sub-command: %s\n", sub_command);
    return 1;
  }
}
