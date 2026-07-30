#include <fstream>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include "job_broker_logger_internal.h"

// Size of buffer to store a passwd entry for getpwnam_r() and getpwuid_r().
#define GETPW_R_BUF_SIZE 8192

//
// Expands a file path.
//
bool
job_broker_expand_path(const std::string& path, std::string& exp_path) {
  bool result = false;

  try {
    if (path.find("~") != 0u) {
      exp_path = path;
      result = true;
    } else {
      char buf[GETPW_R_BUF_SIZE];
      struct passwd pw;
      struct passwd *pw_result = NULL;

      size_t slash_index = path.find("/", 1u);
      if (slash_index == 1u || path.length() == 1u) {
        errno = 0;
        int tmp_err = getpwuid_r(geteuid(), &pw, buf, sizeof(buf), &pw_result);
        if (pw_result != NULL) {
          exp_path = pw.pw_dir;
          if (slash_index != std::string::npos) {
            exp_path += path.substr(slash_index);
          }
          result = true;
        } else if (tmp_err == 0) {
          msg_error("Failed to get an username of the current user\n");
        } else {
          msg_error("Failed to get an username of the current user %s\n", strerror(errno));
        }
      } else {
        std::string username = path.substr(1u, slash_index - 1u);
        errno = 0;
        int tmp_err = getpwnam_r(username.c_str(), &pw, buf, sizeof(buf), &pw_result);
        if (pw_result != NULL) {
          exp_path = pw.pw_dir;
          if (slash_index != std::string::npos) {
            exp_path += path.substr(slash_index);
          }
          result = true;
        } else if (tmp_err == 0) {
          msg_error("Invalid path, no such user: %s\n", username.c_str());
        } else {
          msg_error("Failed to get the password entry: %s\n", strerror(errno));
        }
      }
    }
  } catch (...) {
    msg_error("An exception occurred\n");
  }

  return result;
}

//
// Reads the specified file.
//
bool
job_broker_read_file(const std::string& file_name, std::string& data) {
  bool result = false;

  try {
    std::string exp_file_name;
    if (!job_broker_expand_path(file_name, exp_file_name)) {
      return false;
    }

    auto f = std::ifstream(exp_file_name, std::ios::in | std::ios::binary);
    if (f) {
      f.seekg(0, std::ios::end);
      auto data_size = static_cast<std::size_t>(f.tellg());
      f.seekg(0, std::ios::beg);
      if (f) {
        data.clear();
        data.resize(data_size);
        f.read(&data[0], data.size());
        if (f) {
          result = true;
          msg_debug(5, "Read the file: %s\n", exp_file_name.c_str());
        } else {
          msg_error("Failed to read the file: %s\n", exp_file_name.c_str());
        }
      } else {
        msg_error("Failed to seek the file: %s\n", exp_file_name.c_str());
      }
    } else {
        msg_error("Failed to open the file: %s\n", exp_file_name.c_str());
    }
    f.close();
  } catch (...) {
    msg_error("An exception occurred while reading the file\n");
  }

  return result;
}

//
// Reads the specified file in the given directory.
//
bool
job_broker_read_file_in_dir(const std::string& dir, const std::string& file_name,
                            std::string& data) {
  try {
    std::string path = dir;
    path += "/";
    path += file_name;
    return job_broker_read_file(path, data);
  } catch (...) {
    msg_error("An exception occurred while reading the file in a directory\n");
    return false;
  }
}
