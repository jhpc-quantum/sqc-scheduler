#pragma once

#include <string>

bool job_broker_expand_path(const std::string& path, std::string& exp_path);
bool job_broker_read_file(const std::string& file_name, std::string& data);
bool job_broker_read_file_in_dir(const std::string& dir, const std::string& file_name,
                                 std::string& data);
