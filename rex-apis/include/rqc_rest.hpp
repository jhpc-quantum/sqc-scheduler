#ifndef RQC_REST_HPP_
#define RQC_REST_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class RQCClient {
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

enum class Transpiler : std::uint32_t {
    NONE = 0,
    PASS,
    NORMAL
};

inline const std::string TranspilerToString(Transpiler transpiler) {
  switch (transpiler) {
    case Transpiler::NONE: { return "none"; }
    case Transpiler::PASS: { return "pass"; }
    case Transpiler::NORMAL: { return "normal"; }
    default: { return "none"; }
  }
}

class RQCHttpClient {
public:
    static const std::string kGetJobPath;
    static const std::string kSubmitJobPath;
    static const std::string kIDKey;
    static const std::string kJobIDKey;
    static const std::string kReasonKey;
    static const std::string kStatusKey;
    static const std::string kStatusSuccessValue;
    static const std::string kStatusFailureValue;

    static std::shared_ptr<RQCHttpClient> CreateHttpClient(const std::string& baseUrl);
    static std::shared_ptr<RQCHttpClient> CreateHttpClient(const std::string& baseUrl, const std::string& proxyUrl);

    RQCHttpClient(const std::string& baseUrl) : baseUrl_(baseUrl) {
        httpClient_ = std::make_shared<web::http::client::http_client>(baseUrl_);
    }
    RQCHttpClient(const std::string& baseUrl, const std::string& proxyUrl) : baseUrl_(baseUrl), proxyUrl_(proxyUrl) {
        web::http::client::http_client_config config;
        config.set_proxy(web::web_proxy(proxyUrl));

        httpClient_ = std::make_shared<web::http::client::http_client>(baseUrl_, config);
    }
    ~RQCHttpClient() {}

    std::shared_ptr<JobResult> GetJob(const std::string& path, const std::string& token, const std::string& jobId);
    std::shared_ptr<JobResult> SubmitJob(const std::string& path, const std::string& token,
                                         const std::string& qprogram, const std::uint32_t circuit_fmt,
                                         const std::uint32_t shots,
                                         const std::string& transpiler, const std::string& remark);
    std::shared_ptr<JobResult> CancelJob(const std::string& path, const std::string& token, const std::string& jobId);
    std::shared_ptr<JobResult> DeleteJob(const std::string& path, const std::string& token, const std::string& jobId);

private:
    static const std::string kQApiTokenHeader;
    static const std::string kContentTypeHeader;

    std::string baseUrl_;
    std::string proxyUrl_;
    std::shared_ptr<web::http::client::http_client> httpClient_;
};

}  // namespace rexapis

#endif  // RQC_REST_HPP_

