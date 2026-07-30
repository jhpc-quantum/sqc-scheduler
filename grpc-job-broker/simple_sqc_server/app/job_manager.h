#pragma once

#include <map>
#include <mutex>

#include "blocking_queue.h"
#include "job_invoker.h"

namespace sqc_job {

///
/// Manage jobs with a queue and a map table.
///
class JobManager {
public:
  JobManager() = delete;
  JobManager(const JobManager& other) = delete;
  JobManager& operator=(const JobManager& other) = delete;

  ///
  /// Constructor.
  ///
  /// @param   queue_max_size  the maximum number of jobs stored in the queue.
  /// @param   invoker  job invoker.
  ///
  explicit JobManager(std::size_t queue_max_size, JobInvoker* invoker);

  ///
  /// Destructor.
  ///
  ~JobManager() = default;

  ///
  /// Put a job into the queue.
  ///
  /// @param   job  a job entry.
  ///
  /// The job ID of `job` instance must be filled by the caller.
  ///
  void submit_job(const Job& job);

  ///
  /// Take a job from the queue and invoke it.
  ///
  void invoke_next_job();

  ///
  /// Return an entry stored in the job table.
  ///
  /// @param   job_id  job ID.
  /// @return  an entry the job.
  ///
  Job get_job(const std::string& job_id);

  ///
  /// Return execution result of the job.
  ///
  /// @param   job_id  job ID.
  /// @return  result text.
  ///
  std::string get_job_result(const std::string& job_id);

private:
  BlockingQueue<Job*> queue_;
  std::map<std::string, Job> table_;
  sqc_job::JobInvoker* invoker_;
  std::mutex mutex_;
}; // class JobManager

} // namespace sqc_job
