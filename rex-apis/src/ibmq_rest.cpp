#include <chrono>
#include <thread>

#include "ibmq_json.hpp"
#include "ibmq_rest.hpp"

namespace rexapis {

const std::uint32_t IBMQClient::kWaitJobPollingInterval = 300000;
const std::uint32_t IBMQClient::kWaitJobMaxPollingCount = 50;

const std::string IBMQJobRestClient::kTranspilerServiceURL = "https://cloud-transpiler.quantum.ibm.com";
const std::uint32_t IBMQJobRestClient::kWaitTranspilePollingInterval = 60000;
const std::uint32_t IBMQJobRestClient::kWaitTranspileMaxPollingCount = 10;

const std::string IBMQJobRestClient::kBackend = "ibm_brisbane";
const std::string IBMQJobRestClient::kHub = "ibm-q";
const std::string IBMQJobRestClient::kGroup = "open";
const std::string IBMQJobRestClient::kProject = "main";

const std::string IBMQJobRestClient::kSubmitJobPath = "/jobs";
const std::string IBMQJobRestClient::kGetJobStatusPath = "/jobs/";
const std::string IBMQJobRestClient::kGetJobResultPath = "/jobs/";
const std::string IBMQJobRestClient::kCancelJobPath = "/jobs/";
const std::string IBMQJobRestClient::kDeleteJobPath = "/jobs/";
const std::string IBMQJobRestClient::kTranspilePath = "/transpile";
const std::string IBMQJobRestClient::kGetTranspilationResultsPath = "/transpile/";

const std::string IBMQJobRestClient::kAuthorizationHeader = "Authorization";
const std::string IBMQJobRestClient::kContentTypeHeader = "Content-Type";
const std::string IBMQJobRestClient::kAcceptHeader = "Accept";

std::shared_ptr<JobResult> IBMQClient::Calculate(const std::string& base_url, const std::string& token,
                                                 const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                 const std::uint32_t shots,
                                                 const std::uint32_t transpiler, const std::string& remark,
                                                 std::uint32_t polling_interval, std::uint32_t max_polling_count) {
    // TODO: Add parameter checks

    std::shared_ptr<JobRestClient> client = IBMQJobRestClient::CreateJobRestClient(base_url);

    // submit job
    std::shared_ptr<JobResult> submitResult = client->SubmitJob(token, qprogram, circuit_fmt, shots, transpiler, remark);
    if (submitResult->GetStatusCode() == status_codes::OK) {
        // for debug
//        std::cerr << submitResult->GetJsonBody().c_str() << std::endl;

        // Parse SubmitJob JSON
        IBMQSubmitJobResponseParser parser{};
        auto result = parser.Parse(submitResult->GetJsonBody());
        std::string targetJobId;
        if (std::holds_alternative<SubmitJobValue>(result)) {
            const SubmitJobValue& val = std::get<SubmitJobValue>(result);
            targetJobId = val.GetQCJobId();
        } else if (std::holds_alternative<JsonParseError>(result)) {
            const JsonParseError& err = std::get<JsonParseError>(result);
            std::string msg = err.GetMessage();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
        } else {
            std::string msg = "Failed to parse SubmitJob JSON.";
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
        }

        // wait for the job to run
        for (std::uint32_t i = 0; i < max_polling_count; i++) {


            std::shared_ptr<JobResult> getJobStatusResult = client->GetJobStatus(token, targetJobId);
            if (getJobStatusResult->GetStatusCode() == status_codes::OK) {
                // for debug
//                std::cerr << getJobStatusResult->GetJsonBody().c_str() << std::endl;

                // Parse GetJobStatus JSON
                IBMQGetJobStatusResponseParser parser{targetJobId};
                auto result = parser.Parse(getJobStatusResult->GetJsonBody());
                std::string retStatus;
                if (std::holds_alternative<GetJobStatusValue>(result)) {
                    const GetJobStatusValue& val = std::get<GetJobStatusValue>(result);
                    retStatus = val.GetStatus();
                } else if (std::holds_alternative<JsonParseError>(result)) {
                    const JsonParseError& err = std::get<JsonParseError>(result);
                    std::string msg = err.GetMessage();
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                } else {
                    std::string msg = "Failed to parse GetJobStatus JSON.";
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                }

                if (retStatus == IBMQJson::kStatusCompletedValue) {
                    return client->GetJobResult(token, targetJobId);
                } else if (retStatus == IBMQJson::kStatusFailedValue) {
                    std::string msg = "Job execution failed";
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                } else if (retStatus == IBMQJson::kStatusCancelledValue) {
                    std::string msg = "Job Cancelled";
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                } else {
                    // for debug
                    std::cerr << "Calculate Wait: " << i << std::endl;

                    std::this_thread::sleep_for(std::chrono::milliseconds(polling_interval));
                    continue;
                }
            } else {
                return getJobStatusResult;
            }
        }

        std::cerr << "Job execution failed: Timeout" << std::endl;

        return std::make_shared<JobResult>(web::http::status_codes::RequestTimeout, "", "Calculate(GetJob) Timeout");
    } else {
        return submitResult;
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::SubmitJob(const std::string& token, const std::string& qprogram,
                                                        const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                        const std::uint32_t transpiler, const std::string& remark) {
    // TODO: Add parameter checks
    (void)remark;

    // submit transpile job
    std::string transpiled_qasm = "";
    std::shared_ptr<JobResult> transpileResult = TranspileCircuit(token, qprogram, static_cast<IBMQTranspiler>(transpiler));
    if (transpileResult->GetStatusCode() == status_codes::OK) {
        // for debug
//        std::cerr << transpileResult->GetJsonBody().c_str() << std::endl;

        // Parse TranspileCircuit JSON
        IBMQTranspileCircuitResponseParser parser(transpileResult->GetJsonBody());
        auto result = parser.Parse();
        std::string taskId;
        if (std::holds_alternative<IBMQTranspileCircuitValue>(result)) {
            const IBMQTranspileCircuitValue& val = std::get<IBMQTranspileCircuitValue>(result);
            taskId = val.GetTaskId();
        } else if (std::holds_alternative<JsonParseError>(result)) {
            const JsonParseError& err = std::get<JsonParseError>(result);
            std::string msg = err.GetMessage();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
        } else {
            std::string msg = "Failed to parse TranspileCircuit JSON.";
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
        }

        // wait for the transpile to run
        for (std::uint32_t i = 0; i < kWaitTranspileMaxPollingCount; i++) {
            std::shared_ptr<JobResult> transpilationResult = GetTranspilationResults(token, taskId);
            if (transpilationResult->GetStatusCode() == status_codes::OK) {
                // for debug
//                std::cerr << transpilationResult->GetJsonBody().c_str() << std::endl;

                // Parse GetTranspilationResults JSON
                IBMQGetTranspilationResultsResponseParser parser(taskId, transpilationResult->GetJsonBody());
                auto result = parser.Parse();
                std::string retState = "";
                std::string retResult = "";
                if (std::holds_alternative<IBMQGetTranspilationResultsValue>(result)) {
                    const IBMQGetTranspilationResultsValue& val = std::get<IBMQGetTranspilationResultsValue>(result);
                    retState = val.GetState();
                    retResult = val.GetResult();
                } else if (std::holds_alternative<JsonParseError>(result)) {
                    const JsonParseError& err = std::get<JsonParseError>(result);
                    std::string msg = err.GetMessage();
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                } else {
                    std::string msg = "Failed to parse GetTranspilationResults JSON.";
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                }

                if (retState == IBMQJson::kStateSuccessValue) {
                    transpiled_qasm = retResult;
                    break;
                } else if (retState == IBMQJson::kStateFailureValue) {
                    // for debug
                    std::cerr << "Transpile Failed: " << retResult.c_str() << std::endl;
                    return std::make_shared<JobResult>(0, "", retResult);
                } else {
                    // for debug
                    std::cerr << "Transpile Wait: " << i << std::endl;

                    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitTranspilePollingInterval));
                    continue;
                }
            } else {
                return transpilationResult;
            }
        }
    } else {
        return transpileResult;
    }

    // run job
    return RunJob(token, transpiled_qasm, shots);
}

std::shared_ptr<JobResult> IBMQJobRestClient::GetJobStatus(const std::string& token, const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    // https://cloud.ibm.com/apidocs/quantum-computing#get-job-details-jid
    uri_builder builder(this->baseUrl_);
    builder.set_path(kGetJobStatusPath);
    builder.append_path(qcJobId);

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kAcceptHeader, "application/json");

    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to GetJobStatus: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::GetJobResult(const std::string& token, const std::string& qcJobId) {
    // TODO: Add parameter checks

    // create path
    // https://cloud.ibm.com/apidocs/quantum-computing#get-job-results-jid
    uri_builder builder(this->baseUrl_);
    builder.set_path(kGetJobResultPath);
    builder.append_path(qcJobId);
    builder.append_path("results");

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kAcceptHeader, "application/json");

    // send request
    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to GetJobResult: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::CancelJob(const std::string& token, const std::string& jobId) {
    // TODO: Add parameter checks

    // create path
    // https://cloud.ibm.com/apidocs/quantum-computing#cancel-job-jid
    uri_builder builder(this->baseUrl_);
    builder.set_path(kCancelJobPath);
    builder.append_path(jobId);
    builder.append_path("cancel");

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kAcceptHeader, "application/json");

    // send request
    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::NoContent: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to CancelJob: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::DeleteJob(const std::string& token, const std::string& jobId) {
    // TODO: Add parameter checks

    // create path
    // https://cloud.ibm.com/apidocs/quantum-computing#delete-job-jid
    uri_builder builder(this->baseUrl_);
    builder.set_path(kDeleteJobPath);
    builder.append_path(jobId);

    // create request
    http_request req(methods::DEL);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kAcceptHeader, "application/json");

    // send request
    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::NoContent: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to DeleteJob: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::TranspileCircuit(const std::string &token, const std::string &qprogram,
                                                               const IBMQTranspiler transpiler) {
    // create path
    // https://quantum.cloud.ibm.com/docs/en/api/qiskit-transpiler-service-rest/tags/transpiler-methods
    uri_builder builder(this->transpileUrl_);
    builder.set_path(kTranspilePath);
    builder.append_query("backend=" + kBackend);
    builder.append_query("optimization_level=3");
    builder.append_query("ai=" + IBMQTranspilerUseAI(transpiler));

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kContentTypeHeader, "application/json");
    req.headers().add(kAcceptHeader, "application/json");

    // create JSON
    // https://docs.quantum.ibm.com/api/qiskit-transpiler-service-rest/tags/transpiler-methods#tags__transpiler-methods__operations__transpile_transpile_post
    json::value reqJson;

    json::value circuitsJson;
    circuitsJson[0] = json::value::string(qprogram);

    reqJson["qasm_circuits"] = circuitsJson;

    req.set_body(reqJson.serialize());

    // send request
    http_response res = this->transpileHttpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to TranspileCircuit: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::GetTranspilationResults(const std::string &token, const std::string &taskID) {
    // create path
    // https://quantum.cloud.ibm.com/docs/en/api/qiskit-transpiler-service-rest/tags/transpiler-methods
    uri_builder builder(this->transpileUrl_);
    builder.set_path(kTranspilePath);
    builder.append_path(taskID);

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kAcceptHeader, "application/json");

    // send request
    http_response res = this->transpileHttpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to GetTranspilationResults: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> IBMQJobRestClient::RunJob(const std::string& token,
                                                     const std::string& qprogram, const std::uint32_t shots) {
    // TODO: Add parameter checks

    // create path
    // https://cloud.ibm.com/apidocs/quantum-computing#create-job
    uri_builder builder(this->baseUrl_);
    builder.set_path(kSubmitJobPath);

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kAuthorizationHeader, "Bearer " + token);
    req.headers().add(kContentTypeHeader, "application/json");
    req.headers().add(kAcceptHeader, "application/json");

    // create JSON
    //
    // https://docs.quantum.ibm.com/api/runtime/tags/jobs#tags__jobs__operations__CreateJobController_createJob
    json::value reqJson;
    reqJson["program_id"] = json::value::string("sampler");
    reqJson["backend"] = json::value::string(kBackend); // actual machine settings: ibm_brisbane
    reqJson["hub"] = json::value::string(kHub);    // JSON elements not required for IBM Cloud services
    reqJson["group"] = json::value::string(kGroup);   // JSON elements not required for IBM Cloud services
    reqJson["project"] = json::value::string(kProject); // JSON elements not required for IBM Cloud services

    json::value pubsJson;
    pubsJson[0][0] = json::value::string(qprogram);

    json::value paramsJson;
    paramsJson["pubs"] = pubsJson;
    paramsJson["shots"] = json::value::number(shots);
    paramsJson["version"] = json::value::number(2);

    reqJson["params"] = paramsJson;

    req.set_body(reqJson.serialize());

    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to RunJob: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobRestClient> IBMQJobRestClient::CreateJobRestClient(const std::string& runtimeUrl) {
    return std::make_shared<IBMQJobRestClient>(runtimeUrl);
}

std::shared_ptr<JobRestClient> IBMQJobRestClient::CreateJobRestClient(const std::string& runtimeUrl, const std::string& proxyUrl) {
    return std::make_shared<IBMQJobRestClient>(runtimeUrl, proxyUrl);
}

}  // namespace rexapis

