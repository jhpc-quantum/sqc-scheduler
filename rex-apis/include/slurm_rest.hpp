#ifndef SLURM_REST_HPP_
#define SLURM_REST_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class SlurmClient {
public:
    static std::shared_ptr<JobResult> Calculate(const std::string& baseUrl, const std::string& token,
                                                const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                const std::uint32_t shots,
                                                const std::uint32_t transpiler, const std::string& remark) {
        return Calculate(baseUrl, token, qprogram, circuit_fmt, shots, transpiler, remark, kPollingInterval, kMaxPollingCount);
    }

    static std::shared_ptr<JobResult> Calculate(const std::string& baseUrl, const std::string& token,
                                                const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                const std::uint32_t shots,
                                                const std::uint32_t transpiler, const std::string& remark,
                                                std::uint32_t pollingInterval, std::uint32_t maxPollingCount);

private:
    static const std::uint32_t kPollingInterval;
    static const std::uint32_t kMaxPollingCount;
};

class SlurmJobRestClient : public JobRestClient {
public:
    static const std::string kSubmitJobPath;
    static const std::string kGetJobStatusPath;
    static const std::string kDeleteJobPath;

    static const std::string kStateBootfailValue;
    static const std::string kStateCancelledValue;
    static const std::string kStateCompletedValue;
    static const std::string kStateDeadLineValue;
    static const std::string kStateFailedValue;
    static const std::string kStateNodeFailValue;
    static const std::string kStateOutOfMemoryValue;
    static const std::string kStatePendingValue;
    static const std::string kStatePreemptedValue;
    static const std::string kStateRunningValue;
    static const std::string kStateSuspendedValue;
    static const std::string kStateTimeoutValue;

    static const std::string kScriptTemplate;
    static const std::string kPartitionValue;
    static const std::string kCurrentWorkingDirectoryValue;
    static const std::string kBucketURLValue;

    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& baseUrl);
    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& baseUrl, const std::string& proxyUrl);

    SlurmJobRestClient(const std::string& baseUrl) : JobRestClient(baseUrl) {}
    SlurmJobRestClient(const std::string& baseUrl, const std::string& proxyUrl) : JobRestClient(baseUrl, proxyUrl) {}
    ~SlurmJobRestClient() {}

    virtual std::shared_ptr<JobResult> SubmitJob(const std::string& token, const std::string& qprogram,
                                                 const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                 const std::uint32_t transpiler, const std::string& remark) override;
    virtual std::shared_ptr<JobResult> GetJobStatus(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> GetJobResult(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> CancelJob(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> DeleteJob(const std::string& token, const std::string& qcJobId) override;

    std::string GenerateScript(const std::string qprogram, const std::uint32_t circuit_fmt,
                               const std::uint32_t shots, const std::string bucketUrl);

private:
    static const std::string kUserTokenHeader;
    static const std::string kContentTypeHeader;
};

}  // namespace rexapis

#endif  // SLURM_REST_HPP_

