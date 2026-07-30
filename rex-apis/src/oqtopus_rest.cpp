#include <chrono>
#include <thread>

#include "rexapis_util.hpp"
#include "oqtopus_json.hpp"
#include "oqtopus_rest.hpp"

namespace rexapis {

const std::string OQTOPUSJobRestClient::kRegisterJobPath = "/jobs";
const std::string OQTOPUSJobRestClient::kSubmitJobPath = "/submit";
const std::string OQTOPUSJobRestClient::kJobPath = "/jobs/";
const std::string OQTOPUSJobRestClient::kStatusJobPath = "/status";
const std::string OQTOPUSJobRestClient::kCancelJobPath = "/cancel";

const std::string OQTOPUSJobRestClient::kQApiTokenHeader = "q-api-token";

const std::string OQTOPUSJobRestClient::kQprogramJobInfoKey = "job_info";
const std::string OQTOPUSJobRestClient::kQprogramProgramKey = "program";

const std::string OQTOPUSJobUploadClient::kQApiTokenHeader = "q-api-token";

const std::string OQTOPUSJobUploadClient::kZipOutputDir = "/tmp";

const std::string OQTOPUSJobUploadClient::kBoundary = "--------------------------GenerateS3MultipartBody0987654321";

std::shared_ptr<JobResult> OQTOPUSJobRestClient::SubmitJob(const std::string& token,
                                                           const std::string& qprogram,
                                                           const std::uint32_t circuit_fmt,
                                                           const std::uint32_t shots,
                                                           const std::uint32_t transpiler,
                                                           const std::string& remark) {
    (void)circuit_fmt;
    (void)shots;
    (void)transpiler;
    (void)remark;

    // parse QProgram
    json::value reqJson;
    std::string program;
    try {
        reqJson = json::value::parse(qprogram);

        if (reqJson.has_field(kQprogramJobInfoKey) &&
            reqJson.at(kQprogramJobInfoKey).has_field(kQprogramProgramKey)) {
            json::value program_array = reqJson.at(kQprogramJobInfoKey).at(kQprogramProgramKey);

            json::value program_with_key = json::value::object();
            program_with_key[kQprogramProgramKey] = program_array;
            program = program_with_key.serialize();

            reqJson.erase(kQprogramJobInfoKey);
        } else {
            std::cerr << "SubmitJob Parsing JSON Error: Unsupported JSON format" << std::endl;
            return std::make_shared<JobResult>(web::http::status_codes::BadRequest, "", "Unsupported JSON format");
        }
    } catch (const json::json_exception& e) {
        std::cerr << "SubmitJob Parsing JSON Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::BadRequest, "", std::string(e.what()));
    }

    // call RegisterAndUploadJob
    std::string qcJobId;
    auto registerAndUploadJobResult = RegisterAndUploadJob(token, program);
    if (registerAndUploadJobResult->GetStatusCode() == web::http::status_codes::OK) {
        auto registerResultJson = json::value::parse(registerAndUploadJobResult->GetJsonBody());
        qcJobId = registerResultJson[OQTOPUSJson::kJobIDKey].as_string();
    } else {
        return registerAndUploadJobResult;
    }

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kJobPath);
    builder.append_path(qcJobId);
    builder.append_path(kSubmitJobPath);

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);
    req.headers().set_content_type("application/json");

    // set JSON
    req.set_body(reqJson.serialize());

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                // create JSON containing QC Job ID
                json::value resultJson = json::value::object();
                resultJson[OQTOPUSJson::kJobIDKey] = web::json::value::string(qcJobId);
                return std::make_shared<JobResult>(res.status_code(), resultJson.serialize(), "");
            }
            default: {
                std::cerr << "SubmitJob Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "SubmitJob HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::GetJobStatus(const std::string& token,
                                                              const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kJobPath);
    builder.append_path(qcJobId);
    builder.append_path(kStatusJobPath);

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "GetJobStatus Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "GetJobStatus HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::GetJobResult(const std::string& token,
                                                              const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kJobPath);
    builder.append_path(qcJobId);

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "GetJobResult Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "GetJobResult HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::CancelJob(const std::string& token,
                                                           const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kJobPath);
    builder.append_path(qcJobId);
    builder.append_path(kCancelJobPath);

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "CancelJob Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "CancelJob HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::DeleteJob(const std::string& token,
                                                           const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kJobPath);
    builder.append_path(qcJobId);

    // create request
    http_request req(methods::DEL);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "DeleteJob Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "DeleteJob HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::RegisterAndUploadJob(const std::string& token,
                                                                      const std::string& program) {
    auto registerJobResult = RegisterJob(token);
    if (registerJobResult->GetStatusCode() != web::http::status_codes::OK) {
        return registerJobResult;
    }

    // for debug
//    std::cerr << registerJobResult->GetJsonBody().c_str() << std::endl;

    // Parse RegisterJob JSON
    OQTOPUSRegisterJobResponseParser registerJobParser{};
    auto parsedValue = registerJobParser.Parse(registerJobResult->GetJsonBody());

    if (std::holds_alternative<OQTOPUSRegisterJobValue>(parsedValue)) {
        const OQTOPUSRegisterJobValue& val = std::get<OQTOPUSRegisterJobValue>(parsedValue);

        OQTOPUSJobUploadClient jobUploadHttpClient(val.GetUrl());
        auto uploadJobResult = jobUploadHttpClient.UploadJob(token, program, val.GetQCJobId(), val.GetKey(),
                                                             val.GetAwsAccessKeyId(), val.GetAmzSecurityToken(),
                                                             val.GetPolicy(), val.GetSignature());
        if (uploadJobResult->GetStatusCode() == web::http::status_codes::NoContent) {
            json::value resultJson = json::value::object();
            resultJson[OQTOPUSJson::kJobIDKey] = web::json::value::string(val.GetQCJobId());
            return std::make_shared<JobResult>(web::http::status_codes::OK, resultJson.serialize(), "");
        } else {
            return uploadJobResult;
        }
    } else if (std::holds_alternative<JsonParseError>(parsedValue)) {
        const JsonParseError& val = std::get<JsonParseError>(parsedValue);
        std::cerr << "RegisterJob Parse Error: " << val.GetMessage() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", val.GetMessage());
    } else {
        std::cerr << "RegisterJob Parse Error" << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", "RegisterJob Parse Error");
    }
}

std::shared_ptr<JobResult> OQTOPUSJobRestClient::RegisterJob(const std::string& token) {
    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kRegisterJobPath);

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kQApiTokenHeader, token);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::OK: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "RegisterJob Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "RegisterJob HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

std::shared_ptr<JobRestClient> OQTOPUSJobRestClient::CreateJobRestClient(const std::string& baseUrl) {
    return std::make_shared<OQTOPUSJobRestClient>(baseUrl);
}

std::shared_ptr<JobRestClient> OQTOPUSJobRestClient::CreateJobRestClient(const std::string& baseUrl,
                                                                         const std::string& proxyUrl) {
    return std::make_shared<OQTOPUSJobRestClient>(baseUrl, proxyUrl);
}

std::shared_ptr<JobResult> OQTOPUSJobUploadClient::UploadJob(const std::string& token,
                                                             const std::string& program,
                                                             const std::string& qcJobId,
                                                             const std::string& key,
                                                             const std::string& awsAccessKeyId,
                                                             const std::string& amzSecurityToken,
                                                             const std::string& policy,
                                                             const std::string& signature) {
    // create request
    http_request req(methods::POST);
    req.headers().add(kQApiTokenHeader, token);
    req.headers().set_content_type("multipart/form-data; boundary=" + kBoundary);

    std::string contentFileName = qcJobId + ".json";
    std::string outputFileName = qcJobId + ".zip";
    std::string outputFilePath;
    try {
        outputFilePath = CreateZipFromString(program, contentFileName, kZipOutputDir, outputFileName);
    } catch (const std::exception& e) {
        std::cerr << "UploadJob Create Zip Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }

    std::vector<std::pair<std::string, std::string>> textParams = {
        {OQTOPUSJson::kKeyKey, key},
        {OQTOPUSJson::kAWSAccessKeyIdKey, awsAccessKeyId},
        {OQTOPUSJson::kAmzSecurityTokenKey, amzSecurityToken},
        {OQTOPUSJson::kPolicyKey, policy},
        {OQTOPUSJson::kSignatureKey, signature}
    };

    std::vector<unsigned char> multipartBody;
    try {
        // ZIP file is deleted by the GenerateS3MultipartBody function
        multipartBody = GenerateS3MultipartBody(kBoundary, textParams, outputFileName, outputFilePath);
    } catch (const std::exception& e) {
        std::cerr << "UploadJob Create Multipart Body Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }

    // set multipart body
    req.headers().set_content_length(multipartBody.size());
    req.set_body(multipartBody);

    try {
        http_response res = this->httpClient_->request(req).get();
        auto resString = res.extract_string().get();
        switch (res.status_code()) {
            case web::http::status_codes::NoContent: {
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
            default: {
                std::cerr << "UploadJob Error: " << res.status_code() << ", " << resString << std::endl;
                return std::make_shared<JobResult>(res.status_code(), resString, "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "UploadJob HTTP Request Error: " << e.what() << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", std::string(e.what()));
    }
}

}  // namespace rexapis

