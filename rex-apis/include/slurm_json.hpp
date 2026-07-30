#ifndef SLURM_JSON_HPP_
#define SLURM_JSON_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class SlurmJson {
public:
    // common
    static const std::string kJobIDKey;
    static const std::string kJobsKey;
    static const std::string kStateKey;
};

class SlurmJsonParser : JsonParser {
public:
    static const std::string kResult;
    static const std::string kCounts;

    SlurmJsonParser(const std::string& jsonStr, std::uint32_t shots) : JsonParser(jsonStr, shots) {}
    ~SlurmJsonParser() {}

    virtual std::shared_ptr<JsonResult> Scrape() override;

    static std::shared_ptr<JsonResult> ScrapeResponse(const std::string& jsonStr, std::uint32_t shots);
};

class SlurmSubmitJobResponseParser : public SubmitJobResponseParser {
public:
    SlurmSubmitJobResponseParser() : SubmitJobResponseParser() {}
    ~SlurmSubmitJobResponseParser() {}

    std::variant<SubmitJobValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class SlurmGetJobStatusResponseParser : public GetJobStatusResponseParser {
public:
    SlurmGetJobStatusResponseParser(const std::string& qcJobId) : GetJobStatusResponseParser(qcJobId) {}
    ~SlurmGetJobStatusResponseParser() {}

    std::variant<GetJobStatusValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

class SlurmGetJobResultResponseParser : public GetJobResultResponseParser {
public:
    SlurmGetJobResultResponseParser(const std::string& qcJobId) : GetJobResultResponseParser(qcJobId) {}
    ~SlurmGetJobResultResponseParser() {}

    std::variant<GetJobResultValue, JsonParseError> Parse(const std::string& jsonStr) override;
};

}  // namespace rexapis

#endif  // SLURM_JSON_HPP_

