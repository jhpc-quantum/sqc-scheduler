#ifndef IBMQ_JSON_HPP_
#define IBMQ_JSON_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class IBMQJson {
public:
    // common
    static const std::string kIDKey;
    static const std::string kStatusKey;
    static const std::string kStateKey;
    static const std::string kTaskIDKey;
    static const std::string kResultKey;
    static const std::string kQASMKey;

    static const std::string kStateSuccessValue;
    static const std::string kStateFailureValue;

    static const std::string kStatusCompletedValue;
    static const std::string kStatusFailedValue;
    static const std::string kStatusCancelledValue;
};

class IBMQJsonParser : JsonParser {
public:
    static const std::string kResults;
    static const std::string kData;
    static const std::string kSamples;

    IBMQJsonParser(const std::string& jsonStr, std::uint32_t shots) : JsonParser(jsonStr, shots) {}
    ~IBMQJsonParser() {}

    virtual std::shared_ptr<JsonResult> Scrape() override;

    static std::shared_ptr<JsonResult> ScrapeResponse(const std::string& jsonStr, std::uint32_t shots);
};

class IBMQSubmitJobResponseParser : public SubmitJobResponseParser {
public:
    IBMQSubmitJobResponseParser() : SubmitJobResponseParser() {}
    ~IBMQSubmitJobResponseParser() {}

    std::variant<SubmitJobValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class IBMQGetJobStatusResponseParser : public GetJobStatusResponseParser {
public:
    IBMQGetJobStatusResponseParser(const std::string& qcJobId) : GetJobStatusResponseParser(qcJobId) {}
    ~IBMQGetJobStatusResponseParser() {}

    std::variant<GetJobStatusValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class IBMQGetJobResultResponseParser : public GetJobResultResponseParser {
public:
    IBMQGetJobResultResponseParser(const std::string& qcJobId) : GetJobResultResponseParser(qcJobId) {}
    ~IBMQGetJobResultResponseParser() {}

    std::variant<GetJobResultValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class IBMQTranspileCircuitValue {
public:
    IBMQTranspileCircuitValue(const std::string& taskId) : taskId_(taskId) {}
    ~IBMQTranspileCircuitValue() {}

    std::string GetTaskId() const { return taskId_; }

private:
    std::string taskId_;
};

class IBMQGetTranspilationResultsValue {
public:
    IBMQGetTranspilationResultsValue(const std::string& taskId, const std::string& state,
                                     const std::string& result) : taskId_(taskId), state_(state), result_(result) {}
    ~IBMQGetTranspilationResultsValue() {}

    std::string GetTaskId() const { return taskId_; }
    std::string GetState() const { return state_; }
    std::string GetResult() const { return result_; }

private:
    std::string taskId_;
    std::string state_;
    std::string result_;
};

class IBMQTranspileCircuitResponseParser {
public:
    IBMQTranspileCircuitResponseParser(const std::string& jsonStr) : jsonStr_(jsonStr) {}
    ~IBMQTranspileCircuitResponseParser() {}

    std::variant<IBMQTranspileCircuitValue, JsonParseError> Parse();

private:
    std::string jsonStr_;
};

class IBMQGetTranspilationResultsResponseParser {
public:
    IBMQGetTranspilationResultsResponseParser(const std::string& taskId, const std::string& jsonStr) : taskId_(taskId), jsonStr_(jsonStr) {}
    ~IBMQGetTranspilationResultsResponseParser() {}

    std::variant<IBMQGetTranspilationResultsValue, JsonParseError> Parse();

private:
    std::string taskId_;
    std::string jsonStr_;
};

}  // namespace rexapis

#endif  // IBMQ_JSON_HPP_

