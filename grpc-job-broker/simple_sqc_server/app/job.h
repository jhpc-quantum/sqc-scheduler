#pragma once

#include <optional>
#include <string>
#include <string>
#include <cstddef>
#include <cstdint>

namespace sqc_job {

///
/// Status of a job.
///
enum class JobStatus {
  Unknown = 0,
  Created = 1,
  Queued = 2,
  Running = 3,
  Done = 4,
  Cancelled = 5,
  Error = 6,
  Deleted = 7,
}; // enum class JobStatus

///
/// A job.
///
class Job {
public:
  ///
  /// Constructor.
  ///
  /// All string data members are set to empty and all data members of integer types are set to 0.
  ///
  Job();

  ///
  /// Constructor.
  ///
  /// ID will be assigned automatically.  It sets `qc_job_id` to empty and sets status to `Created`.
  ///
  Job(const std::string& user_id, const std::string& group_id, std::uint32_t priority,
      const std::string& qprogram, int circuit_fmt, std::size_t shots, int qc_type,
      int transpiler, const std::string& remark, std::optional<std::string> user_token);

  ///
  /// Constructor.
  ///
  /// It sets all data members in an instance by the given arguments.
  ///
  Job(const std::string& user_id, const std::string& group_id, std::uint32_t priority,
      const std::string& qprogram, int circuit_fmt, std::size_t shots, int qc_type,
      int transpiler, const std::string& remark, std::optional<std::string> user_token,
      const std::string& id, const std::string& qc_job_id, JobStatus status);

  ///
  /// Copy constructor.
  ///
  Job(const Job& other) = default;

  ///
  /// Destructor.
  ///
  ~Job() = default;

  ///
  /// Copy assignment operator.
  ///
  Job& operator=(const Job& other) = default;

  inline const std::string& user_id() const noexcept { return user_id_; }
  inline const std::string& group_id() const noexcept { return group_id_; }
  inline std::uint32_t priority() const noexcept { return priority_; }
  inline const std::string& qprogram() const noexcept { return qprogram_; }
  inline int circuit_fmt() const noexcept { return circuit_fmt_; }
  inline std::size_t shots() const noexcept { return shots_; }
  inline int qc_type() const noexcept { return qc_type_; }
  inline int transpiler() const noexcept { return transpiler_; }
  inline const std::string& remark() const noexcept { return remark_; }
  inline const std::optional<std::string>& user_token() const noexcept { return user_token_; }
  inline const std::string& id() const noexcept { return id_; }
  inline const std::string& qc_job_id() const noexcept { return qc_job_id_; }
  inline void set_qc_job_id(const std::string& value) { qc_job_id_ = value; }
  inline JobStatus status() const noexcept { return status_; }
  inline void set_status(JobStatus value) noexcept { status_ = value; }

private:
  std::string user_id_;
  std::string group_id_;
  std::uint32_t priority_;
  std::string qprogram_;
  int circuit_fmt_;
  std::size_t shots_;
  int qc_type_;
  int transpiler_;
  std::string remark_;
  std::optional<std::string> user_token_;
  std::string id_;
  std::string qc_job_id_;
  JobStatus status_;
}; // class Job;

} // namespace sqc_job
