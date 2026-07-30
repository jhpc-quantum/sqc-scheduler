#include "oqtopus_json.hpp"

namespace rexapis {

const std::string OQTOPUSJson::kJobIDKey = "job_id";
const std::string OQTOPUSJson::kStatusKey = "status";
const std::string OQTOPUSJson::kMessageKey = "message";
const std::string OQTOPUSJson::kPresignedURLKey = "presigned_url";
const std::string OQTOPUSJson::kURLKey = "url";
const std::string OQTOPUSJson::kFieldsKey = "fields";
const std::string OQTOPUSJson::kKeyKey = "key";
const std::string OQTOPUSJson::kAWSAccessKeyIdKey = "AWSAccessKeyId";
const std::string OQTOPUSJson::kAmzSecurityTokenKey = "x-amz-security-token";
const std::string OQTOPUSJson::kPolicyKey = "policy";
const std::string OQTOPUSJson::kSignatureKey = "signature";

std::variant<SubmitJobValue, JsonParseError> OQTOPUSSubmitJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto jobId = resJson[OQTOPUSJson::kJobIDKey].as_string();
        return SubmitJobValue(jobId);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobStatusValue, JsonParseError> OQTOPUSGetJobStatusResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto status = resJson[OQTOPUSJson::kStatusKey].as_string();
        return GetJobStatusValue(this->qcJobId_, status);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<GetJobResultValue, JsonParseError> OQTOPUSGetJobResultResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto result = resJson.serialize();
        return GetJobResultValue(this->qcJobId_, result);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<CancelJobValue, JsonParseError> OQTOPUSCancelJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto msg = resJson[OQTOPUSJson::kMessageKey].as_string();
        return CancelJobValue(this->qcJobId_, msg);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<DeleteJobValue, JsonParseError> OQTOPUSDeleteJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);
        auto msg = resJson[OQTOPUSJson::kMessageKey].as_string();
        return DeleteJobValue(this->qcJobId_, msg);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

std::variant<OQTOPUSRegisterJobValue, JsonParseError> OQTOPUSRegisterJobResponseParser::Parse(const std::string& jsonStr) {
    try {
        auto resJson = json::value::parse(jsonStr);

        auto jobId = resJson[OQTOPUSJson::kJobIDKey].as_string();

        auto presignedURLObj = resJson[OQTOPUSJson::kPresignedURLKey].as_object();
        auto url = presignedURLObj[OQTOPUSJson::kURLKey].as_string();
        auto fieldsObj = presignedURLObj[OQTOPUSJson::kFieldsKey].as_object();

        auto key = fieldsObj[OQTOPUSJson::kKeyKey].as_string();
        auto awsAccessKeyId = fieldsObj[OQTOPUSJson::kAWSAccessKeyIdKey].as_string();
        auto amzSecurityToken = fieldsObj[OQTOPUSJson::kAmzSecurityTokenKey].as_string();
        auto policy = fieldsObj[OQTOPUSJson::kPolicyKey].as_string();
        auto signature = fieldsObj[OQTOPUSJson::kSignatureKey].as_string();
        return OQTOPUSRegisterJobValue(jobId, url, key, awsAccessKeyId ,amzSecurityToken, policy, signature);
    } catch (const std::exception& e) {
        return JsonParseError(e.what());
    }
}

}  // namespace rexapis

