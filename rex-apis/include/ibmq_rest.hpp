#ifndef IBMQ_REST_HPP_
#define IBMQ_REST_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class IBMQClient {
public:
    static std::shared_ptr<JobResult> Calculate(const std::string& baseUrl, const std::string& token,
                                                const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                const std::uint32_t shots,
                                                const std::uint32_t transpiler, const std::string& remark) {
        return Calculate(baseUrl, token, qprogram, circuit_fmt, shots,
                         transpiler, remark, kWaitJobPollingInterval, kWaitJobMaxPollingCount);
    }

    static std::shared_ptr<JobResult> Calculate(const std::string& baseUrl, const std::string& token,
                                                const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                const std::uint32_t shots,
                                                const std::uint32_t transpiler, const std::string& remark,
                                                std::uint32_t pollingInterval, std::uint32_t maxPollingCount);

private:
    static const std::uint32_t kWaitJobPollingInterval;
    static const std::uint32_t kWaitJobMaxPollingCount;
};

enum class IBMQTranspiler : std::uint32_t {
    NONE = 0,
    NOAI = 1,
    AI = 2
};

inline const std::string IBMQTranspilerUseAI(IBMQTranspiler transpiler) {
    switch (transpiler) {
        case IBMQTranspiler::AI: { return "true"; }
        default: { return "false"; }
    }
}

class IBMQJobRestClient : public JobRestClient {
public:
    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& runtimeUrl);
    static std::shared_ptr<JobRestClient> CreateJobRestClient(const std::string& runtimeUrl, const std::string& proxyUrl);

    IBMQJobRestClient(const std::string& runtimeUrl) : JobRestClient(runtimeUrl), transpileUrl_(kTranspilerServiceURL) {
        transpileHttpClient_ = std::make_shared<web::http::client::http_client>(kTranspilerServiceURL);
    }
    IBMQJobRestClient(const std::string& runtimeUrl, const std::string& proxyUrl)
        : JobRestClient(runtimeUrl, proxyUrl), transpileUrl_(kTranspilerServiceURL) {
        web::http::client::http_client_config config;
        config.set_proxy(web::web_proxy(proxyUrl));

        transpileHttpClient_ = std::make_shared<web::http::client::http_client>(kTranspilerServiceURL, config);
    }
    ~IBMQJobRestClient() {}

    virtual std::shared_ptr<JobResult> SubmitJob(const std::string& token, const std::string& qprogram,
                                                 const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                 const std::uint32_t transpiler, const std::string& remark) override;
    virtual std::shared_ptr<JobResult> GetJobStatus(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> GetJobResult(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> CancelJob(const std::string& token, const std::string& qcJobId) override;
    virtual std::shared_ptr<JobResult> DeleteJob(const std::string& token, const std::string& qcJobId) override;
    std::shared_ptr<JobResult> TranspileCircuit(const std::string& token, const std::string& qprogram, const IBMQTranspiler transpiler);
    std::shared_ptr<JobResult> GetTranspilationResults(const std::string& token, const std::string& task_id);
    std::shared_ptr<JobResult> RunJob(const std::string& token, const std::string& qprogram, const std::uint32_t shots);

private:
    static const std::string kTranspilerServiceURL;
    static const std::uint32_t kWaitTranspilePollingInterval;
    static const std::uint32_t kWaitTranspileMaxPollingCount;

    static const std::string kBackend;
    static const std::string kHub;
    static const std::string kGroup;
    static const std::string kProject;

    static const std::string kSubmitJobPath;
    static const std::string kGetJobStatusPath;
    static const std::string kGetJobResultPath;
    static const std::string kCancelJobPath;
    static const std::string kDeleteJobPath;
    static const std::string kTranspilePath;
    static const std::string kGetTranspilationResultsPath;

    static const std::string kAuthorizationHeader;
    static const std::string kContentTypeHeader;
    static const std::string kAcceptHeader;

    std::string transpileUrl_;
    std::shared_ptr<web::http::client::http_client> transpileHttpClient_;
};

}  // namespace rexapis

#endif  // IBMQ_REST_HPP_

