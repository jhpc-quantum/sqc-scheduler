#include <chrono>
#include <thread>

#include "slurm_json.hpp"
#include "slurm_rest.hpp"
#include "minio_cli.hpp"

namespace rexapis {

const std::uint32_t SlurmClient::kPollingInterval = 1000;
const std::uint32_t SlurmClient::kMaxPollingCount = 60;

const std::string SlurmJobRestClient::kUserTokenHeader = "X-SLURM-USER-TOKEN";
const std::string SlurmJobRestClient::kContentTypeHeader = "Content-Type";

const std::string SlurmJobRestClient::kSubmitJobPath = "/job/submit";
const std::string SlurmJobRestClient::kGetJobStatusPath = "/job";
const std::string SlurmJobRestClient::kDeleteJobPath = "/job";

// Job state(https://slurm.schedmd.com/job_state_codes.html)
const std::string SlurmJobRestClient::kStateBootfailValue = "BOOT_FAIL";
const std::string SlurmJobRestClient::kStateCancelledValue = "CANCELLED";
const std::string SlurmJobRestClient::kStateCompletedValue = "COMPLETED";
const std::string SlurmJobRestClient::kStateDeadLineValue = "DEADLINE";
const std::string SlurmJobRestClient::kStateFailedValue = "FAILED";
const std::string SlurmJobRestClient::kStateNodeFailValue = "NODE_FAIL";
const std::string SlurmJobRestClient::kStateOutOfMemoryValue = "OUT_OF_MEMORY";
const std::string SlurmJobRestClient::kStatePendingValue = "PENDING";
const std::string SlurmJobRestClient::kStatePreemptedValue = "PREEMPTED";
const std::string SlurmJobRestClient::kStateRunningValue = "RUNNING";
const std::string SlurmJobRestClient::kStateSuspendedValue = "SUSPENDED";
const std::string SlurmJobRestClient::kStateTimeoutValue = "TIMEOUT";

const std::string SlurmJobRestClient::kScriptTemplate = R"(#!/bin/bash
echo $(date)
SLURM_JOB_ID=$SLURM_JOB_ID
srun python3 sim.py '${QASM}' --shots=${SHOTS} --path=results/result-$SLURM_JOB_ID.json --bucket-url=${BUCKET_URL}
RESULT=$?
exit $RESULT
)";
const std::string SlurmJobRestClient::kPartitionValue = "";
const std::string SlurmJobRestClient::kCurrentWorkingDirectoryValue = "";
const std::string SlurmJobRestClient::kBucketURLValue = "";

std::shared_ptr<JobResult> SlurmClient::Calculate(const std::string& baseUrl, const std::string& token,
                                                  const std::string& qprogram, const std::uint32_t circuit_fmt,
                                                  const std::uint32_t shots,
                                                  const std::uint32_t transpiler, const std::string& remark,
                                                  std::uint32_t pollingInterval, std::uint32_t maxPollingCount) {
    // TODO: Add parameter checks

    std::shared_ptr<JobRestClient> client = SlurmJobRestClient::CreateJobRestClient(baseUrl);

    // submit job
    std::shared_ptr<JobResult> submitResult = client->SubmitJob(token, qprogram, circuit_fmt, shots, transpiler, remark);
    if (submitResult->GetStatusCode() == web::http::status_codes::OK) {
        // for debug
        std::cerr << submitResult->GetJsonBody().c_str() << std::endl;

        // Parse SubmitJob JSON
        SlurmSubmitJobResponseParser parser{};
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

        // for debug
        std::cerr << "Target Job ID: " << targetJobId << std::endl;

        for (std::uint32_t i = 0; i < maxPollingCount; i++) {
            std::shared_ptr<JobResult> getJobResult = client->GetJobStatus(token, targetJobId);
            if (getJobResult->GetStatusCode() == web::http::status_codes::OK) {
                // for debug
                std::cerr << getJobResult->GetJsonBody().c_str() << std::endl;

                // Parse GetJobStatus JSON
                SlurmGetJobStatusResponseParser parser{targetJobId};
                auto result = parser.Parse(getJobResult->GetJsonBody());
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
                    std::string msg = "Failed to parse GetJob JSON.";
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                }

                // Check job state and wait for job to finish
                if (retStatus == SlurmJobRestClient::kStateCompletedValue) { // Job is finished(success)
                    return MinioClient::GetResult(SlurmJobRestClient::kCurrentWorkingDirectoryValue, targetJobId);
                } else if (retStatus == SlurmJobRestClient::kStateBootfailValue ||
                           retStatus == SlurmJobRestClient::kStateCancelledValue ||
                           retStatus == SlurmJobRestClient::kStateDeadLineValue ||
                           retStatus == SlurmJobRestClient::kStateFailedValue ||
                           retStatus == SlurmJobRestClient::kStateNodeFailValue ||
                           retStatus == SlurmJobRestClient::kStateOutOfMemoryValue ||
                           retStatus == SlurmJobRestClient::kStateTimeoutValue) { // Job is finished(fail)
                    std::string msg = "Job execution failed: " + retStatus;
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                } else if (retStatus == SlurmJobRestClient::kStatePendingValue ||
                           retStatus == SlurmJobRestClient::kStatePreemptedValue ||
                           retStatus == SlurmJobRestClient::kStateRunningValue ||
                           retStatus == SlurmJobRestClient::kStateSuspendedValue) { // Job is running
                    std::this_thread::sleep_for(std::chrono::milliseconds(pollingInterval));
                    continue;
                } else {
                    std::string msg = "Unknown status: " + retStatus;
                    std::cerr << msg << std::endl;
                    return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", msg);
                }
            } else {
                std::string msg = "Failed to execute GetJob";
                std::cerr << msg << std::endl;
                return std::make_shared<JobResult>(getJobResult->GetStatusCode(), "", msg);
            }
        }

        std::cerr << "Job execution failed: Timeout" << std::endl;

        return std::make_shared<JobResult>(web::http::status_codes::RequestTimeout, "", "Calculate(JobsStatus) Timeout");
    } else {
        return submitResult;
    }
}

std::shared_ptr<JobResult> SlurmJobRestClient::SubmitJob(const std::string& token, const std::string& qprogram,
                                                         const std::uint32_t circuit_fmt, const std::uint32_t shots,
                                                         const std::uint32_t transpiler, const std::string& remark) {
    // TODO: Add parameter checks
    (void)transpiler;
    (void)remark;

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kSubmitJobPath);

    // create request
    http_request req(methods::POST);
    req.set_request_uri(builder.to_string());
    req.headers().add(kUserTokenHeader, token);
    req.headers().add(kContentTypeHeader, "application/json");

    // create JSON
    json::value environmentJson = web::json::value::array();
    environmentJson[0] = web::json::value::string(U("PATH=/usr/bin:/usr/local/bin"));
    environmentJson[1] = web::json::value::string(U("PYTHONPATH=$PYTHONPATH:site-packages/"));

    json::value jobJson;
    jobJson["name"] = json::value::string("REX REST JOB");
    jobJson["partition"] = json::value::string(SlurmJobRestClient::kPartitionValue);
    jobJson["current_working_directory"] = json::value::string(SlurmJobRestClient::kCurrentWorkingDirectoryValue);
    jobJson["standard_output"] = json::value::string("results/myjob-%j.stdout");
    jobJson["standard_error"] = json::value::string("results/myjob-%j.stderr");
    jobJson["environment"] = environmentJson;
    const std::string script = GenerateScript(qprogram, circuit_fmt, shots, SlurmJobRestClient::kBucketURLValue);
    jobJson["script"] = json::value::string(script);

    json::value reqJson;
    reqJson["job"] = jobJson;

    // for debug
//    std::cerr << "Json: " << reqJson.serialize() << std::endl;

    req.set_body(reqJson.serialize());

    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
            auto resString = res.extract_string().get();
            return std::make_shared<JobResult>(res.status_code(), resString, "");
        }
        default: {
            std::string msg = "Failed to SubmitJob: " + std::to_string(res.status_code()) + " " + res.extract_string().get();
            std::cerr << msg << std::endl;
            return std::make_shared<JobResult>(res.status_code(), "", msg);
        }
    }
}

std::shared_ptr<JobResult> SlurmJobRestClient::GetJobStatus(const std::string& token, const std::string& jobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kGetJobStatusPath);
    builder.append_path(jobId);

    // create request
    http_request req(methods::GET);
    req.set_request_uri(builder.to_string());
    req.headers().add(kUserTokenHeader, token);

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

std::shared_ptr<JobResult> SlurmJobRestClient::GetJobResult(const std::string& token, const std::string& jobId) {
    // TODO: Add parameter checks

    (void)token;
    return MinioClient::GetResult(kCurrentWorkingDirectoryValue, jobId);
}

// Use Delete for Cancel in Slurm.
// see: https://slurm.schedmd.com/rest_api.html#slurmV0042DeleteJob
std::shared_ptr<JobResult> SlurmJobRestClient::CancelJob(const std::string& token, const std::string& jobId) {
    // TODO: Add parameter checks

    // create path
    uri_builder builder(this->baseUrl_);
    builder.set_path(kDeleteJobPath);
    builder.append_path(jobId);

    // create request
    http_request req(methods::DEL);
    req.set_request_uri(builder.to_string());
    req.headers().add(kUserTokenHeader, token);

    http_response res = this->httpClient_->request(req).get();
    switch (res.status_code()) {
        case web::http::status_codes::OK: {
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

std::shared_ptr<JobResult> SlurmJobRestClient::DeleteJob(const std::string& token, const std::string& jobId) {
    // Unsupported operation
    return std::make_shared<JobResult>(0, "", "Unsupported operation");
}

std::string SlurmJobRestClient::GenerateScript(const std::string qprogram, const std::uint32_t circuit_fmt,
                                               const std::uint32_t shots, const std::string bucketUrl) {
    std::string script = SlurmJobRestClient::kScriptTemplate;

    size_t pos;
    while ((pos = script.find("${QASM}")) != std::string::npos) {
        script.replace(pos, 7, qprogram);
    }
    while ((pos = script.find("${SHOTS}")) != std::string::npos) {
        script.replace(pos, 8, std::to_string(shots));
    }
    while ((pos = script.find("${BUCKET_URL}")) != std::string::npos) {
        script.replace(pos, 13, bucketUrl);
    }

    return script;
}

std::shared_ptr<JobRestClient> SlurmJobRestClient::CreateJobRestClient(const std::string& baseUrl) {
    return std::make_shared<SlurmJobRestClient>(baseUrl);
}

std::shared_ptr<JobRestClient> SlurmJobRestClient::CreateJobRestClient(const std::string& baseUrl, const std::string& proxyUrl) {
    return std::make_shared<SlurmJobRestClient>(baseUrl, proxyUrl);
}

}  // namespace rexapis

