#include <uuid/uuid.h>
#include "job.h"

namespace sqc_job {

Job::Job()
  : user_id_(),
    priority_(0u),
    qprogram_(),
    circuit_fmt_(0),
    shots_(0u),
    qc_type_(0),
    transpiler_(0),
    remark_(),
    id_(),
    qc_job_id_(),
    status_(JobStatus::Unknown) {
}

Job::Job(const std::string& user_id, std::uint32_t priority, const std::string& qprogram, int circuit_fmt,
         std::size_t shots, int qc_type, int transpiler, const std::string& remark, std::optional<std::string> user_token)
  : user_id_(user_id),
    priority_(priority),
    qprogram_(qprogram),
    circuit_fmt_(circuit_fmt),
    shots_(shots),
    qc_type_(qc_type),
    transpiler_(transpiler),
    remark_(remark),
    user_token_(user_token),
    id_(),
    qc_job_id_(),
    status_(JobStatus::Created) {
  uuid_t bin;
  uuid_generate_random(bin);
  char txt[UUID_STR_LEN];
  uuid_unparse_lower(bin, txt);
  id_ = txt;
}

Job::Job(const std::string& user_id, std::uint32_t priority, const std::string& qprogram, int circuit_fmt,
         std::size_t shots, int qc_type, int transpiler, const std::string& remark, std::optional<std::string> user_token,
         const std::string& id, const std::string& qc_job_id, JobStatus status)
  : user_id_(user_id),
    priority_(priority),
    qprogram_(qprogram),
    circuit_fmt_(circuit_fmt),
    shots_(shots),
    qc_type_(qc_type),
    transpiler_(transpiler),
    remark_(remark),
    user_token_(user_token),
    id_(id),
    qc_job_id_(qc_job_id),
    status_(status) {
}

} // namespace sqc_job
