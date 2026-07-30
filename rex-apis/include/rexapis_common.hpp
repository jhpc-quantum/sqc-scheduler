#ifndef REXAPIS_COMMON_HPP_
#define REXAPIS_COMMON_HPP_

#include <chrono>
#include <variant>
#include <vector>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

namespace rexapis {

class JobResult {
public:
    JobResult(const unsigned short statusCode, const std::string jsonBody)
            : statusCode_(statusCode), jsonBody_(jsonBody), errMsg_("") {}
    JobResult(const unsigned short statusCode, const std::string jsonBody,
              const std::string errMsg)
            : statusCode_(statusCode), jsonBody_(jsonBody), errMsg_(errMsg) {}
    ~JobResult() {}

    unsigned short GetStatusCode() { return statusCode_; }
    std::string GetJsonBody() { return jsonBody_; }
    std::string GetErrMsg() { return errMsg_; }

private:
    unsigned short statusCode_;
    std::string jsonBody_;
    std::string errMsg_;
};

class RestClient {
public:
    RestClient(const std::string& baseUrl) : baseUrl_(baseUrl) {
        web::http::client::http_client_config config;
        config.set_timeout(std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS));

        httpClient_ = std::make_shared<web::http::client::http_client>(baseUrl, config);
    }
    RestClient(const std::string& baseUrl, const std::string& proxyUrl) : baseUrl_(baseUrl), proxyUrl_(proxyUrl) {
        web::http::client::http_client_config config;
        config.set_timeout(std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS));
        config.set_proxy(web::web_proxy(proxyUrl));

        httpClient_ = std::make_shared<web::http::client::http_client>(baseUrl, config);
    }
    virtual ~RestClient() = default;

protected:
    std::string baseUrl_;
    std::string proxyUrl_;
    std::shared_ptr<web::http::client::http_client> httpClient_;

private:
    static constexpr int DEFAULT_TIMEOUT_SECONDS = 30;
};

class JobRestClient : public RestClient {
public:
    JobRestClient(const std::string& baseUrl) : RestClient(baseUrl) {}
    JobRestClient(const std::string& baseUrl, const std::string& proxyUrl) : RestClient(baseUrl, proxyUrl) {}
    virtual ~JobRestClient() = default;

    virtual std::shared_ptr<JobResult> SubmitJob(const std::string& token, const std::string& qprogram,
                                                 const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                 const std::uint32_t transpiler, const std::string& remark) = 0;
    virtual std::shared_ptr<JobResult> GetJobStatus(const std::string& token, const std::string& qcJobId) = 0;
    virtual std::shared_ptr<JobResult> GetJobResult(const std::string& token, const std::string& qcJobId) = 0;
    virtual std::shared_ptr<JobResult> CancelJob(const std::string& token, const std::string& qcJobId) = 0;
    virtual std::shared_ptr<JobResult> DeleteJob(const std::string& token, const std::string& qcJobId) = 0;
};

class JsonResult {
public:
    JsonResult() : patterns_{}, probs_{}, size_(0) {}
    ~JsonResult() {}

    void AddResult(int pattern, float prob) {
        patterns_.push_back(pattern);
        probs_.push_back(prob);
        size_++;
    }
    std::vector<int> GetPatterns() { return patterns_; };
    std::vector<float> GetProbs() { return probs_; };
    std::uint32_t GetSize() { return size_; };

private:
    std::vector<int> patterns_;
    std::vector<float> probs_;
    std::uint32_t size_;
};

class JsonParser {
public:
    JsonParser(const std::string& jsonStr, std::uint32_t shots) : jsonStr_(jsonStr), shots_(shots) {}
    ~JsonParser() {}

    virtual std::shared_ptr<JsonResult> Scrape() = 0;

protected:
    std::string jsonStr_;
    std::uint32_t shots_;
};

class SubmitJobValue {
public:
    SubmitJobValue(const std::string& qcJobId) : qcJobId_(qcJobId) {}
    ~SubmitJobValue() {}

    std::string GetQCJobId() const { return qcJobId_; }

private:
    std::string qcJobId_;
};

class GetJobStatusValue {
public:
    GetJobStatusValue(const std::string& qcJobId, const std::string& status) : qcJobId_(qcJobId), status_(status) {}
    ~GetJobStatusValue() {}

    std::string GetQCJobId() const { return qcJobId_; }
    std::string GetStatus() const { return status_; }

private:
    std::string qcJobId_;
    std::string status_;
};

class GetJobResultValue {
public:
    GetJobResultValue(const std::string& qcJobId, const std::string& result) : qcJobId_(qcJobId), result_(result) {}
    ~GetJobResultValue() {}

    std::string GetQCJobId() const { return qcJobId_; }
    std::string GetResult() const { return result_; }

private:
    std::string qcJobId_;
    std::string result_;
};

class CancelJobValue {
public:
    CancelJobValue(const std::string& qcJobId, const std::string& msg) : qcJobId_(qcJobId), msg_(msg) {}
    ~CancelJobValue() {}

    std::string GetQCJobId() const { return qcJobId_; }
    std::string GetMsg() const { return msg_; }

private:
    std::string qcJobId_;
    std::string msg_;
};

class DeleteJobValue {
public:
    DeleteJobValue(const std::string& qcJobId, const std::string& msg) : qcJobId_(qcJobId), msg_(msg) {}
    ~DeleteJobValue() {}

    std::string GetQCJobId() const { return qcJobId_; }
    std::string GetMsg() const { return msg_; }

private:
    std::string qcJobId_;
    std::string msg_;
};

class JsonParseError {
public:
    JsonParseError(const std::string& msg) : msg_(msg) {}
    ~JsonParseError() {}

    std::string GetMessage() const { return msg_; }

private:
    std::string msg_;
};

class SubmitJobResponseParser {
public:
    SubmitJobResponseParser() {}
    virtual ~SubmitJobResponseParser() = default;

    virtual std::variant<SubmitJobValue, JsonParseError> Parse(const std::string& jsonStr) = 0;
};

class GetJobStatusResponseParser {
public:
    GetJobStatusResponseParser(const std::string& qcJobId) : qcJobId_(qcJobId) {}
    virtual ~GetJobStatusResponseParser() = default;

    virtual std::variant<GetJobStatusValue, JsonParseError> Parse(const std::string& jsonStr) = 0;

protected:
    std::string qcJobId_;
};

class GetJobResultResponseParser {
public:
    GetJobResultResponseParser(const std::string& qcJobId) : qcJobId_(qcJobId) {}
    virtual ~GetJobResultResponseParser() = default;

    virtual std::variant<GetJobResultValue, JsonParseError> Parse(const std::string& jsonStr) = 0;

protected:
    std::string qcJobId_;
};

class CancelJobResponseParser {
public:
    CancelJobResponseParser(const std::string& qcJobId) : qcJobId_(qcJobId) {}
    virtual ~CancelJobResponseParser() = default;

    virtual std::variant<CancelJobValue, JsonParseError> Parse(const std::string& jsonStr) = 0;

protected:
    std::string qcJobId_;
};

class DeleteJobResponseParser {
public:
    DeleteJobResponseParser(const std::string& qcJobId) : qcJobId_(qcJobId) {}
    virtual ~DeleteJobResponseParser() = default;

    virtual std::variant<DeleteJobValue, JsonParseError> Parse(const std::string& jsonStr) = 0;

protected:
    std::string qcJobId_;
};

}  // namespace rexapis

#endif // REXAPIS_COMMON_HPP_

