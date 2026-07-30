#include <stdexcept>
#include "job_manager.h"

namespace sqc_job {

JobManager::JobManager(std::size_t queue_max_size, JobInvoker* invoker)
  : queue_(queue_max_size),
    table_(),
    invoker_(invoker),
    mutex_() {
  if (invoker_ == nullptr) {
    throw std::runtime_error("invoker is nullptr");
  }
}

void JobManager::submit_job(const Job& job) {
  Job* placed_job = nullptr;
  try {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto pair = table_.emplace(job.id(), job);
      if (!pair.second) {
        throw std::runtime_error("job " + job.id() + " already exists");
      }
      placed_job = &((pair.first)->second);
      placed_job->set_status(JobStatus::Queued);
    }
    queue_.enqueue(placed_job);
  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (placed_job != nullptr) {
      placed_job->set_status(JobStatus::Error);
    }
    throw;
  }
}

void JobManager::invoke_next_job() {
  Job* job = nullptr;
  try {
    job = queue_.dequeue();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      job->set_status(JobStatus::Running);
    }
    invoker_->invoke(*job);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      job->set_status(JobStatus::Done);
    }
  } catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (job != nullptr) {
      job->set_status(JobStatus::Error);
    }
    throw;
  }
}

Job JobManager::get_job(const std::string& job_id) {
  try {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto iter = table_.find(job_id);
      if (iter == table_.end()) {
        return Job();
      }
      return iter->second;
    }
  } catch (const std::exception& e) {
    throw;
  }
}

std::string JobManager::get_job_result(const std::string& job_id) {
  try {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto iter = table_.find(job_id);
      if (iter == table_.end()) {
        return std::string();
      }
    }
    return invoker_->get_result(job_id);
  } catch (const std::exception& e) {
    throw;
  }
}

} // namespace sqc_job
