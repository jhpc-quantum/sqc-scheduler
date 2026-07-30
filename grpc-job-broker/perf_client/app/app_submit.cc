#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <optional>

#include "job_broker.pb.h"
#include "job_broker_perf_client.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"
#include "app.h"

typedef struct perf_args {
  uint32_t thread_id;
  pthread_barrier_t *barrier;

  std::string server;
  std::string conf_dir;
  std::uint32_t priority;
  std::string qprogram;
  circuit_fmt_t circuit_fmt;
  std::size_t shots;
  qc_type_t qc_type;
  transpiler_t transpiler;
  std::string remark;
  std::optional<std::string> user_token;
  uint32_t loop_count;
} perf_args_t;

static inline std::uint64_t get_nanotime() noexcept {
  struct timespec ts;
  if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000u + ts.tv_nsec;
  } else {
    return 0u;
  }
}

//
// Sends a 'submit_job' request to gRPC server.
//
static bool
s_submit_job(job_broker_perf_client& client, std::uint32_t priority, const std::string& qprogram,
             circuit_fmt_t circuit_fmt, std::size_t shots, qc_type_t qc_type, transpiler_t transpiler,
             const std::string& remark, std::optional<std::string> user_token,
             const std::uint32_t thread_id, const std::uint32_t loop_count) {
  std::uint64_t submit_start = 0;
  std::uint64_t submit_end = 0;

  // Sends a 'submit_job' request.
  submit_job_reply reply;
  submit_start = get_nanotime();
  grpc::Status grpc_status = client.submit_job(qprogram, circuit_fmt, shots, qc_type, transpiler,
                                               remark, user_token, priority, reply);
  submit_end = get_nanotime();

  // Parses the reply.
  if (!grpc_status.ok()) {
    msg_error("gRPC error: %s\n", grpc_status.error_message().c_str());
    return false;
  }

  std::int64_t result_code = reply.code();
  if (result_code == RESULT_OK && reply.has_job_id()) {
    msg_info("[PERF] [thread:%d] [loop:%d] [submit] %f nsec, %.3f usec, %.6f msec, code: %d, job_id: %s, "
             "priority=%u, shots=%zu, qc_type=%d, transpiler=%d, remark=%s\n",
             thread_id,
             loop_count,
             (double)(submit_end - submit_start),
             (double)(submit_end - submit_start) / 1000.0,
             (double)(submit_end - submit_start) / 1000.0 / 1000.0,
             static_cast<int>(result_code), reply.job_id().c_str(), priority, shots, qc_type, transpiler, remark.c_str());
  } else {
    msg_info("[PERF] [thread:%d] [loop:%d] [submit] %f nsec, %.3f usec, %.6f msec, err: %s, "
             "priority=%u, shots=%zu, qc_type=%d, transpiler=%d, remark=%s\n",
             thread_id,
             loop_count,
             (double)(submit_end - submit_start),
             (double)(submit_end - submit_start) / 1000.0,
             (double)(submit_end - submit_start) / 1000.0 / 1000.0,
             reply.message().c_str(), priority, shots, qc_type, transpiler, remark.c_str());
  }

  if (result_code != RESULT_OK) {
    return false;
  } else if (!reply.has_job_id()) {
    msg_error("server replies OK, but job ID is not issued\n");
    return false;
  }
  return true;
}

void*
thread_entry(void *arg) {
  perf_args_t *args = (perf_args_t *)arg;

  int ret = pthread_barrier_wait(args->barrier);
  if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
    std::cerr << "failed pthread_barrier_wait: " << strerror(ret) << std::endl;
    pthread_exit(NULL);
  }

  job_broker_perf_client* client = create_job_broker_perf_client(args->server, args->conf_dir);
  if (client == nullptr) {
    std::cerr << "failed create_job_broker_perf_client" << std::endl;
    pthread_exit(NULL);
  }

  for (uint32_t i = 0; i < args->loop_count; i++) {
    (void)s_submit_job(*client, args->priority, args->qprogram, args->circuit_fmt, args->shots, args->qc_type,
                       args->transpiler, args->remark, args->user_token, args->thread_id, i);
  }

  delete client;
  pthread_exit(NULL);
}

//
// Prints help message for 'submit' sub-command.
//
void
s_print_help_submit(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " submit [OPTION...] QC-TYPE PRIORITY QPROGRAMi FORMAT SHOTS"
            << std::endl;
  std::cout << "       " << prog << " submit --help"
            << std::endl
            << std::endl;

  std::cout << "Options:" << std::endl;
  std::cout << "  --conf-dir=DIR       directory where JWT token file exists" << std::endl;
  std::cout << "                       (default: " << get_default_conf_dir() << ")"
            << std::endl;
  std::cout << "  --server=HOST:PORT   gRPC server (default: env $SQC_GRPC_SERVER)"
            << std::endl;
  std::cout << "  --transpiler=TYPE    transpiler; none, pass or normal"
            << std::endl;
  std::cout << "                       (default: none)" << std::endl;
  std::cout << "  --remark=TEXT        remark text (default: empty text)" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << "  QC-TYPE              QC type of the job" << std::endl;
  std::cout << "                       (rqc-rest, ibm-rest or slurm-rest)" << std::endl;
  std::cout << "  PRIORITY             priority of the job" << std::endl;
  std::cout << "  QPROGRAM             path to a qprogram file" << std::endl;
  std::cout << "  FORMAT               format type of QPROGRAM (QASM, QIR or QPY)" << std::endl;
  std::cout << "  SHOTS                the number of shots" << std::endl;
}

//
// Main function for 'submit' sub command.
//
int
do_subcmd_submit(int argc, char* argv[], int optind) {
  std::string server = get_default_server();
  std::string conf_dir = get_default_conf_dir();
  transpiler_t transpiler = TRANSPILER_NONE;
  const std::string  remark = "perf-remark";
  const char* env_user_token = getenv("SQC_GRPC_USER_TOKEN");
  std::optional<std::string> user_token;
  std::uint32_t thread_num = 1u;
  std::uint32_t loop_count = 1u;

  int rc;
  pthread_barrier_t barrier;
  bool barrier_inited = false;

  for (; optind < argc; optind++) {
    std::string arg = argv[optind];
    std::string opt_val;
    if (arg == "--") {
      optind += 1;
      break;
    } else if (arg.compare(0u, 1, "-") != 0) {
      break;
    } else if (arg == "--help") {
      s_print_help_submit(argv[0]);
      return 0;
    } else if (parse_option_with_value(arg, "--server", server)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--conf-dir", conf_dir)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--thread-num", opt_val)) {
      if (!parse_uint32(opt_val, &thread_num)) {
        std::cerr << "Invalid value for thread num: " << opt_val << std::endl;
        return 1;
      }
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--loop-count", opt_val)) {
      if (!parse_uint32(opt_val, &loop_count)) {
        std::cerr << "Invalid value for loop count: " << opt_val << std::endl;
        return 1;
      }
      ; // nothing to do.
    } else {
      std::cerr << "Invalid option '" << arg << "'" << std::endl;
      return 1;
    }
  }

  // Gets the non-option arguments.
  if (optind + 5 != argc) {
    std::cerr << "The invalid number of arguments given to 'submit'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  qc_type_t qc_type = QC_TYPE_UNKNOWN;
  if (!parse_qc_type(argv[optind], qc_type)) {
    std::cerr << "Invalid value for QC-TYPE: " << argv[optind] << std::endl;
    return false;
  }

  std::uint32_t priority = 0u;
  try {
    priority = static_cast<std::uint32_t>(std::stoul(argv[optind + 1]));
  } catch (...) {
    std::cerr << "Invalid value for PRIORITY: " << argv[optind + 1] << std::endl;
    return false;
  }

  std::string qprogram;
  std::string qprogram_file = argv[optind + 2];
  if (!job_broker_read_file(qprogram_file, qprogram)) {
    return false;
  }

  circuit_fmt_t circuit_fmt = CIRCUIT_FMT_UNKNOWN;
  if (!parse_circuit_fmt(argv[optind + 3], circuit_fmt)) {
    std::cerr << "Invalid value for FORMAT: " << argv[optind] << std::endl;
    return false;
  }

  std::size_t shots = 0u;
  try {
    shots = static_cast<std::size_t>(std::stoul(argv[optind + 4]));
  } catch (...) {
    std::cerr << "Invalid value for SHOTS: " << argv[optind + 4] << std::endl;
    return false;
  }

  user_token = env_user_token ? std::optional<std::string>{env_user_token} : std::nullopt;

  msg_info("submit_job request: priority=%u, qprogram_file=%s, shots=%zu, qc_type=%d, "
           "transpiler=%d, remark=%s, user_token=%s, server=%s, conf_dir=%s, "
           "thread_num=%d, loop_count=%d\n",
           priority, qprogram_file.c_str(), shots, qc_type, transpiler, remark.c_str(),
           user_token.value_or("null").c_str(), server.c_str(), conf_dir.c_str(), thread_num, loop_count);

  std::vector<perf_args_t> perf_args(thread_num);
  std::vector<pthread_t> threads(thread_num);

  rc = pthread_barrier_init(&barrier, NULL, thread_num);
  if (rc != 0) {
    std::cerr << "failed pthread_barrier_init: " << strerror(rc) << std::endl;
    goto end;
  }
  barrier_inited = true;

  // create thread
  for (uint32_t i = 0; i < thread_num; i++) {
    perf_args[i].thread_id = i;
    perf_args[i].barrier = &barrier;
    perf_args[i].server = server;
    perf_args[i].conf_dir = conf_dir;
    perf_args[i].priority = priority;
    perf_args[i].qprogram = qprogram;
    perf_args[i].circuit_fmt = circuit_fmt;
    perf_args[i].shots = shots;
    perf_args[i].qc_type = qc_type;
    perf_args[i].transpiler = transpiler;
    perf_args[i].remark = remark;
    perf_args[i].user_token = user_token;
    perf_args[i].loop_count = loop_count;

    rc = pthread_create(&threads[i], NULL, thread_entry, &perf_args[i]);
    if (rc != 0) {
      std::cerr << "failed pthread_create: " << strerror(rc) << std::endl;
      goto end;
    }
  }

  // wait thread
  for (uint32_t i = 0; i < thread_num; i++) {
    pthread_join(threads[i], NULL);
  }

end:
  if (barrier_inited) {
    pthread_barrier_destroy(&barrier);
  }
  return rc;
}
