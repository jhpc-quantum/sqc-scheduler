#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "job_broker_perf_client.h"

//
// Converts QC type to enum qc_type_t.
//
bool
parse_qc_type(const std::string& qc_type_str, qc_type_t& qc_type);

//
// Converts tranpiler to enum transpiler_t.
//
bool
parse_transpiler(const std::string& transpiler_str, transpiler_t& transpiler);

//
// Converts cuircuit format to enum circuit_fmt_t.
//
bool
parse_circuit_fmt(const std::string& circuit_fmt_str, circuit_fmt_t& circuit_fmt);

//
// Parses an option with the value.
// It assumes the option has the form '--<option name>=<value>'.
//
bool
parse_option_with_value(const std::string& arg, const std::string& option_name,
                        std::string& option_value);

bool
parse_uint32(const std::string& arg, std::uint32_t *value);

//
// Creates a gRPC client.
//
job_broker_perf_client*
create_job_broker_perf_client(const std::string& server, const std::string& conf_dir);

//
// Prints a log message to stderr.
//
void
emit_log(int log_level, uint64_t debug_level, const char* file, int line,
         const char* func, const char* fmt, ...);

//
// Get location of the default server.
//
const std::string
get_default_server();

//
// Get basename of the given path.
//
std::string
get_path_basename(const std::string& path);

//
// Get path to the default configuration directory.
//
const std::string
get_default_conf_dir();

//
// Main function of sub commands.
//
int
do_subcmd_submit(int argc, char* argv[], int optind);
