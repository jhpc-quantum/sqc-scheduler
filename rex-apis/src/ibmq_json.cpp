#include "ibmq_json.hpp"

namespace rexapis {

const std::string IBMQJson::kIDKey = "id";
const std::string IBMQJson::kStatusKey = "status";
const std::string IBMQJson::kStateKey = "state";
const std::string IBMQJson::kTaskIDKey = "task_id";
const std::string IBMQJson::kResultKey = "result";
const std::string IBMQJson::kQASMKey = "qasm";

const std::string IBMQJson::kStateSuccessValue = "SUCCESS";
const std::string IBMQJson::kStateFailureValue = "FAILURE";

const std::string IBMQJson::kStatusCompletedValue = "Completed";
const std::string IBMQJson::kStatusFailedValue = "Failed";
const std::string IBMQJson::kStatusCancelledValue = "Cancelled";

const std::string IBMQJsonParser::kResults = "results";
const std::string IBMQJsonParser::kData = "data";
const std::string IBMQJsonParser::kSamples = "samples";

std::shared_ptr<JsonResult> IBMQJsonParser::ScrapeResponse(const std::string& jsonStr,
                                                           std::uint32_t shots) {
    std::shared_ptr<IBMQJsonParser> parser = std::make_shared<IBMQJsonParser>(jsonStr, shots);
    return parser->Scrape();
}

std::shared_ptr<JsonResult> IBMQJsonParser::Scrape() {
    std::map<int, float> counts;
    std::shared_ptr<JsonResult> result = std::make_shared<JsonResult>();

    auto json = json::value::parse(this->jsonStr_);
    auto data = json[kResults][0][kData].as_object();

    // iterate over measured classical registers
    for (auto const& it: data) {
        json::value regData = it.second;

        // iterate over all samples (given as an array of hexadecimal values)
        for (auto const& s: regData["samples"].as_array()) {
            int key = std::strtol(s.as_string().c_str(), nullptr, 16);

            if (counts.find(key) != counts.end()) {
                counts[key] += 1;
            } else {
                counts[key] = 1;
            }
        }
    }

    for (auto const& it: counts) {
        result->AddResult(it.first, it.second);
    }

    return result;
}

std::variant<SubmitJobValue, JsonParseError> IBMQSubmitJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto jobId = resJson[IBMQJson::kIDKey].as_string();
        return SubmitJobValue(jobId);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobStatusValue, JsonParseError> IBMQGetJobStatusResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto status = resJson[IBMQJson::kStatusKey].as_string();
        return GetJobStatusValue(this->qcJobId_, status);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobResultValue, JsonParseError> IBMQGetJobResultResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto result = resJson.serialize();
        return GetJobResultValue(this->qcJobId_, result);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<IBMQTranspileCircuitValue, JsonParseError> IBMQTranspileCircuitResponseParser::Parse() {
    try {
        auto resJson = json::value::parse(this->jsonStr_);
        auto taskId = resJson[IBMQJson::kTaskIDKey].as_string();
        return IBMQTranspileCircuitValue(taskId);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<IBMQGetTranspilationResultsValue, JsonParseError> IBMQGetTranspilationResultsResponseParser::Parse() {
    try {
        auto resJson = json::value::parse(this->jsonStr_);
        auto state = resJson[IBMQJson::kStateKey].as_string();
        std::string result = "";
        if (state == IBMQJson::kStateSuccessValue) {
            result = resJson[IBMQJson::kResultKey][0][IBMQJson::kQASMKey].as_string();
        } else if (state == IBMQJson::kStateFailureValue) {
            result = resJson[IBMQJson::kResultKey].as_string();
        }

        return IBMQGetTranspilationResultsValue(this->taskId_, state, result);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

}  // namespace rexapis

