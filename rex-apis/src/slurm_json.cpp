#include "slurm_json.hpp"

namespace rexapis {

const std::string SlurmJson::kJobIDKey = "job_id";
const std::string SlurmJson::kJobsKey = "jobs";
const std::string SlurmJson::kStateKey = "job_state";

const std::string SlurmJsonParser::kResult = "result";
const std::string SlurmJsonParser::kCounts = "counts";

std::shared_ptr<JsonResult> SlurmJsonParser::ScrapeResponse(const std::string& jsonStr,
                                                            std::uint32_t shots) {
    std::shared_ptr<SlurmJsonParser> parser = std::make_shared<SlurmJsonParser>(jsonStr, shots);
    return parser->Scrape();
}

std::shared_ptr<JsonResult> SlurmJsonParser::Scrape() {
    auto json = json::value::parse(this->jsonStr_);

    auto resultStr = json[SlurmJsonParser::kResult].as_string();
    auto resultJson = json::value::parse(resultStr);

    std::shared_ptr<JsonResult> result = std::make_shared<JsonResult>();

    auto countsObj = resultJson[SlurmJsonParser::kCounts].as_object();
    for (auto counts = countsObj.cbegin(); counts != countsObj.cend(); ++counts) {
        int key = std::stoi(counts->first, 0, 2);
        float value = counts->second.as_integer();

        result->AddResult(key, value);
    }

    return result;
}

std::variant<SubmitJobValue, JsonParseError> SlurmSubmitJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto jobId = std::to_string(resJson[SlurmJson::kJobIDKey].as_integer());
        return SubmitJobValue(jobId);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobStatusValue, JsonParseError> SlurmGetJobStatusResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto jobs = resJson[SlurmJson::kJobsKey].as_array();
        for (auto& job : jobs) {
            auto retJobIdVal = job[SlurmJson::kJobIDKey];

            std::string retJobJd;
            if (retJobIdVal.is_string()) {
                retJobJd = retJobIdVal.as_string();
            } else {
                retJobJd = std::to_string(retJobIdVal.as_integer());
            }

            if (retJobJd == this->qcJobId_) {
                auto status = job[SlurmJson::kStateKey][0].as_string();
                return GetJobStatusValue(this->qcJobId_, status);
            }
        }

        return JsonParseError("Job not found: " + this->qcJobId_);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobResultValue, JsonParseError> SlurmGetJobResultResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto result = resJson.serialize();
        return GetJobResultValue(this->qcJobId_, result);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

}  // namespace rexapis

