#include <uuid/uuid.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include "job_broker_logger_internal.h"
#include "job_invoker.h"

namespace sqc_job {

PyJobInvoker::PyJobInvoker(const std::string& py_module, const std::string& py_func, const std::string& spool_dir)
  : py_module_(nullptr),
    py_func_(nullptr),
    py_gstate_(),
    spool_dir_(spool_dir) {
  PyGILState_STATE py_gstate_ = PyGILState_Ensure();

  try {
    py_module_ = PyImport_ImportModule(py_module.c_str());
    if (py_module_ == nullptr) {
      PyErr_Print();
      throw std::runtime_error("failed to load the Python module " + py_module);
    }
    py_func_ = PyObject_GetAttrString(py_module_, py_func.c_str());
    if (py_func_ == nullptr) {
      PyErr_Print();
      throw std::runtime_error("failed to get attr of the Python function " + py_func);
    }
  } catch (const std::exception& e) {
    Py_CLEAR(py_func_);
    Py_CLEAR(py_module_);
    PyGILState_Release(py_gstate_);
    throw;
  }
}

PyJobInvoker::~PyJobInvoker() {
  Py_CLEAR(py_func_);
  Py_CLEAR(py_module_);
  PyGILState_Release(py_gstate_);
}

void PyJobInvoker::invoke(const Job& job) {
  msg_debug(1, "exectue the job id=%s\n", job.id().c_str());
  PyObject* py_args = nullptr;
  PyObject* py_result = nullptr;

  try {
    auto result_file = result_file_path(job.id());
    auto error_file = error_file_path(job.id());
    py_args = Py_BuildValue("(IsiKiisss)",
                            static_cast<unsigned int>(job.priority()),
                            job.qprogram().c_str(),
                            job.circuit_fmt(),
                            static_cast<unsigned long long>(job.shots()),
                            job.qc_type(),
                            job.transpiler(),
                            job.remark().c_str(),
                            result_file.c_str(),
                            error_file.c_str());
    if (py_args == nullptr) {
      throw std::runtime_error("failed to build arguments passed to Python script");
    }

    py_result = PyObject_CallObject(py_func_, py_args);
    if (py_result == nullptr) {
      PyErr_Print();
      msg_info("job id=%s failed to call the object\n", job.id().c_str());
      throw std::runtime_error("the job id=" + job.id() + " failed to call the object");
    }

    const auto succeeded = PyTuple_GetItem(py_result, 0);
    const auto error_message = PyUnicode_AsUTF8AndSize(PyTuple_GetItem(py_result, 1), nullptr);

    if (succeeded == Py_False) {
      msg_info("job id=%s failed: %s\n", job.id().c_str(), error_message);
      throw std::runtime_error("the job id=" + job.id() + " was run but failed");
    } else {
      msg_info("job id=%s succeeded\n", job.id().c_str());
    }
  } catch (const std::exception& e) {
    Py_CLEAR(py_args);
    Py_CLEAR(py_result);
    throw;
  }

  msg_debug(1, "the job id=%s succeeded\n", job.id().c_str());

  Py_CLEAR(py_args);
  Py_CLEAR(py_result);
}

std::string PyJobInvoker::get_result(const std::string& job_id) {
  auto result_file = result_file_path(job_id);
  return read_file(result_file);
}

std::string PyJobInvoker::get_error(const std::string& job_id) {
  auto error_file = error_file_path(job_id);
  return read_file(error_file);
}

std::string PyJobInvoker::result_file_path(const std::string& job_id) const {
  auto filename = job_id + ".result";
  return std::filesystem::path(spool_dir_) / filename;
}

std::string PyJobInvoker::error_file_path(const std::string& job_id) const {
  auto filename = job_id + ".error";
  return std::filesystem::path(spool_dir_) / filename;
}

std::string PyJobInvoker::read_file(const std::string& path) const {
  std ::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open the file " + path);
  } else {
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
  }
}

} // namespace sqc_job
