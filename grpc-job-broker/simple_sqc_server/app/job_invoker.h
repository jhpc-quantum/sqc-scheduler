#pragma once

#include <string>
#include <Python.h>
#include "job.h"

namespace sqc_job {

///
/// Job executor interface.
///
class JobInvoker {
public:
  ///
  /// Destructor.
  ///
  virtual ~JobInvoker() = default;

  ///
  /// Invoke a job.
  ///
  /// @param   job  a job to invoke.
  ///
  /// It thows an exception when the job execution is failed.
  ///
  virtual void invoke(const Job& job) = 0;

  ///
  /// Get execution result of the job.
  ///
  /// @param   job_id  job ID.
  /// @return  result text.
  ///
  virtual std::string get_result(const std::string& job_id) = 0;

  ///
  /// Get an error message during the job execution.
  ///
  /// @param   job_id  job ID.
  /// @return  an error message.
  ///
  virtual std::string get_error(const std::string& job_id) = 0;
}; // class JobInvoker


///
/// Job executor that invokes a Python script to invoke the job.
///
class PyJobInvoker: public JobInvoker {
public:
  PyJobInvoker() = delete;
  PyJobInvoker(const PyJobInvoker& other) = delete;
  PyJobInvoker& operator=(const PyJobInvoker& other) = delete;

  ///
  /// Create an executor.
  ///
  /// @param   py_module  Python module to load.
  /// @param   py_func    Python function to invoke the job.
  /// @param   spool_dir  path to a spool directory.
  ///
  /// The constructor throws an exception when it fails to load Python module `py_module` or
  /// the loaded module doesn't have `py_func`.  Unlike `python` command, it doesn't load a module
  /// at the current directory (`.`) by default.
  ///
  /// Before creating an instance, it is required to call `Py_Initialize()` and `PyEval_SaveThread()`
  /// once each in that order in an application.
  /// When the application exits, it is required to destruct all `PyJobInvoker` instances, and then
  /// call `Py_Finalize()`.
  ///
  PyJobInvoker(const std::string& py_module, const std::string& py_func, const std::string& spool_dir);

  ///
  /// Destructor.
  ///
  virtual ~PyJobInvoker();

  ///
  /// Invoke a job.
  ///
  /// @param   job  a job to invoke.
  ///
  /// It thows an exception when the job execution is failed.
  ///
  virtual void invoke(const Job& job);

  ///
  /// Get execution result of the job.
  ///
  /// @param   job_id  job ID.
  /// @return  result text.
  ///
  virtual std::string get_result(const std::string& job_id);

  ///
  /// Get an error message during the job execution.
  ///
  /// @param   job_id  job ID.
  /// @return  an error message.
  ///
  virtual std::string get_error(const std::string& job_id);

private:
  ///
  /// Return a path to the file which records execution results of the job.
  ///
  /// @param   job_id  job ID.
  /// @return  a path to the result file corresponding with the job ID.
  ///
  std::string result_file_path(const std::string& job_id) const;

  ///
  /// Return a path to the file which records error messages during exection of the job.
  ///
  /// @param   job_id  job ID.
  /// @return  a path to the error file corresponding with the job ID.
  ///
  std::string error_file_path(const std::string& job_id) const;

  ///
  /// Read the entire file.
  ///
  /// @param   path  a path to the file to read.
  /// @return  content read from the file.
  ///
  std::string read_file(const std::string& path) const;

  PyObject* py_module_;
  PyObject* py_func_;
  PyGILState_STATE py_gstate_;
  std::string spool_dir_;
}; // class PyJobInvoker

} // namespace sqc_job
