#ifndef OQTOPUS_REST_HPP_
#define OQTOPUS_REST_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class OQTOPUSJobRestClient : public JobRestClient {
public:
    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& baseUrl);
    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& baseUrl, const std::string& proxyUrl);

    OQTOPUSJobRestClient(const std::string& baseUrl) : JobRestClient(baseUrl) {}
    OQTOPUSJobRestClient(const std::string& baseUrl, const std::string& proxyUrl) : JobRestClient(baseUrl, proxyUrl) {}
    ~OQTOPUSJobRestClient() {}

    virtual std::shared_ptr<JobResult> SubmitJob(const std::string& token, const std::string& qprogram,
                                                 const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                 const std::uint32_t transpiler, const std::string& remark) override;
    virtual std::shared_ptr<JobResult> GetJobStatus(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> GetJobResult(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> CancelJob(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> DeleteJob(const std::string& token, const std::string& qcJobId) override;
    std::shared_ptr<JobResult> RegisterAndUploadJob(const std::string& token, const std::string& program);
    std::shared_ptr<JobResult> RegisterJob(const std::string& token);

private:
    static const std::string kRegisterJobPath;
    static const std::string kSubmitJobPath;
    static const std::string kJobPath;
    static const std::string kStatusJobPath;
    static const std::string kCancelJobPath;

    static const std::string kQApiTokenHeader;

    static const std::string kQprogramJobInfoKey;
    static const std::string kQprogramProgramKey;

    static const std::string kZipOutputDir;

    static const std::string kBoundary;
};

class OQTOPUSJobUploadClient : public RestClient {
public:
    OQTOPUSJobUploadClient(const std::string& baseUrl) : RestClient(baseUrl) {}
    OQTOPUSJobUploadClient(const std::string& baseUrl, const std::string& proxyUrl) : RestClient(baseUrl, proxyUrl) {}
    ~OQTOPUSJobUploadClient() {}

    std::shared_ptr<JobResult> UploadJob(const std::string& token,
                                         const std::string& program,
                                         const std::string& qcJobId,
                                         const std::string& key,
                                         const std::string& awsAccessKeyId,
                                         const std::string& amzSecurityToken,
                                         const std::string& policy,
                                         const std::string& signature);

private:
    static const std::string kQApiTokenHeader;

    static const std::string kZipOutputDir;

    static const std::string kBoundary;
};

}  // namespace rexapis

#endif  // OQTOPUS_REST_HPP_

