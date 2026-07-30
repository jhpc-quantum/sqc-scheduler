#include "rexapis.h"
#include "rqc_rest.hpp"
#include "rqc_json.hpp"
#include "ibmq_rest.hpp"
#include "ibmq_json.hpp"
#include "slurm_rest.hpp"
#include "slurm_json.hpp"
#include "oqtopus_rest.hpp"
#include "oqtopus_json.hpp"

int calculate(uint32_t qc_type, const char *base_url, const char *token, const char *qprogram,
              uint32_t circuit_fmt, uint32_t shots, uint32_t transpiler, const char *remark,
              uint32_t polling_interval, uint32_t max_polling_count,
              char **output, size_t *output_len) {
    std::shared_ptr<rexapis::JobResult> result;
    try {
        if (qc_type == REXAPIS_QC_TYPE_RQC_REST) {
            result = rexapis::RQCClient::Calculate(base_url, token, qprogram, circuit_fmt,
                                                   shots, transpiler, remark,
                                                   polling_interval, max_polling_count);
        } else if (qc_type == REXAPIS_QC_TYPE_IBM_REST) {
            result = rexapis::IBMQClient::Calculate(base_url, token, qprogram, circuit_fmt,
                                                    shots, transpiler, remark,
                                                    polling_interval, max_polling_count);
        } else if (qc_type == REXAPIS_QC_TYPE_SLURM_REST) {
            result = rexapis::SlurmClient::Calculate(base_url, token, qprogram, circuit_fmt,
                                                     shots, transpiler, remark,
                                                     polling_interval, max_polling_count);
        } else {
            fprintf(stderr, "Unsupported QC type: %d\n", qc_type);
            return -1;
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to calculate: %s\n", e.what());
        return -2;
    }

    if (result->GetStatusCode() == web::http::status_codes::OK) {
        size_t result_len = result->GetJsonBody().length() + 1;
        bool allocated = false;
        if (*output == NULL) {
            *output = (char *)malloc(sizeof(char) * result_len);
            if (*output == NULL) {
                fprintf(stderr, "Failed to malloc.\n");
                return -3;
            }
            allocated = true;
        }

        int len = snprintf(*output, result_len, "%s", result->GetJsonBody().c_str());
        if (len < 0) {
            if (allocated == true) {
                free(*output);
                *output = NULL;
            }
            fprintf(stderr, "Failed to output results.\n");
            return -4;
        }

        *output_len = len;

        return 0;
    } else {
        fprintf(stderr, "Invalid status code: %d\n", result->GetStatusCode());
        return -5;
    }
}

int scrape_response(uint32_t qc_type, const char *resp, uint32_t shots, int **patterns,
                    float **probs, size_t *n_patterns) {
    std::shared_ptr<rexapis::JsonResult> result;
    try {
        if (qc_type == REXAPIS_QC_TYPE_RQC_REST) {
            result = rexapis::RQCJsonParser::ScrapeResponse(resp, shots);
        } else if (qc_type == REXAPIS_QC_TYPE_IBM_REST) {
            result = rexapis::IBMQJsonParser::ScrapeResponse(resp, shots);
        } else if (qc_type == REXAPIS_QC_TYPE_SLURM_REST) {
            result = rexapis::SlurmJsonParser::ScrapeResponse(resp, shots);
        } else {
            fprintf(stderr, "Unsupported QC type: %d\n", qc_type);
            return -1;
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "JSON Parse Error: %s\n", e.what());
        return -2;
    }

    std::uint32_t resultSize = result->GetSize();

    if (*patterns == NULL && *probs == NULL) {
        *patterns = (int *)malloc(sizeof(int) * resultSize);
        if (*patterns == NULL) {
            fprintf(stderr, "Failed to malloc.\n");
            return -3;
        }

        *probs = (float *)malloc(sizeof(float) * resultSize);
        if (*probs == NULL) {
            fprintf(stderr, "Failed to malloc.\n");
            return -4;
        }
    } else if (*patterns != NULL && *probs != NULL) {
        resultSize = std::min(shots, resultSize);
    } else {
        fprintf(stderr, "Invalid argument.\n");
        return -5;
    }

    std::vector<int> jsonPatterns = result->GetPatterns();
    std::vector<float> jsonProbs = result->GetProbs();

    for (int i = 0; i < resultSize; i++) {
        (*patterns)[i] = jsonPatterns[i];
        (*probs)[i] = jsonProbs[i];
    }

    *n_patterns = resultSize;

    return 0;
}

int submit_job(uint32_t qc_type, const char *base_url, const char *token, const char *qprogram,
               uint32_t circuit_fmt, uint32_t shots, uint32_t transpiler, const char *remark,
               char **qc_job_id, size_t *qc_job_id_len, char **err_msg, size_t *err_msg_len) {
    int retCode = REXAPIS_RESULT_ANY_FAILURES;
    std::string errMsg = "";
    try {
        std::shared_ptr<rexapis::JobRestClient> client = nullptr;
        std::shared_ptr<rexapis::SubmitJobResponseParser> parser = nullptr;
        if (qc_type == REXAPIS_QC_TYPE_IBM_REST) {
            client = rexapis::IBMQJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::IBMQSubmitJobResponseParser>();
        } else if (qc_type == REXAPIS_QC_TYPE_SLURM_REST) {
            client = rexapis::SlurmJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::SlurmSubmitJobResponseParser>();
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSSubmitJobResponseParser>();
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSSubmitJobResponseParser>();
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSSubmitJobResponseParser>();
        }

        if (client != nullptr && parser != nullptr) {
            auto submitResult = client->SubmitJob(token, qprogram, circuit_fmt, shots, transpiler, remark);
            if (submitResult->GetStatusCode() == web::http::status_codes::OK) {
                // for debug
//                std::cerr << submitResult->GetJsonBody().c_str() << std::endl;

                // Parse SubmitJob JSON
                auto parsedValue = parser->Parse(submitResult->GetJsonBody());
                if (std::holds_alternative<rexapis::SubmitJobValue>(parsedValue)) {
                    const rexapis::SubmitJobValue& val = std::get<rexapis::SubmitJobValue>(parsedValue);
                    auto qcJobId = val.GetQCJobId();
                    *qc_job_id_len = qcJobId.length();

                    *qc_job_id = static_cast<char*>(std::malloc((*qc_job_id_len + 1) * sizeof(char)));
                    if (*qc_job_id == nullptr) {
                        return REXAPIS_RESULT_NO_MEMORY;
                    }
                    std::strcpy(*qc_job_id, qcJobId.c_str());

                    return REXAPIS_RESULT_OK;
                } else if (std::holds_alternative<rexapis::JsonParseError>(parsedValue)) {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(parsedValue);
                    errMsg = err.GetMessage();
                } else {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    errMsg = "Failed to parse SubmitJob JSON.";
                }
            } else {
                retCode = REXAPIS_RESULT_RUNTIME_ERROR;
                errMsg = submitResult->GetErrMsg();
            }
        } else { // RQC is unsupported. Because there is no test environment.
            retCode = REXAPIS_RESULT_UNSUPPORTED;
            errMsg = "Unsupported QC type: " + std::to_string(qc_type);
        }
    } catch (const std::exception& e) {
        retCode = REXAPIS_RESULT_ANY_FAILURES;
        errMsg = "Failed to submit job: " + std::string(e.what());
    }

    *err_msg_len = errMsg.length();

    *err_msg = static_cast<char*>(std::malloc((*err_msg_len + 1) * sizeof(char)));
    if (*err_msg == nullptr) {
        return REXAPIS_RESULT_NO_MEMORY;
    }
    std::strcpy(*err_msg, errMsg.c_str());

    return retCode;
}

int get_job_status(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
                   char **status, size_t *status_len, char **err_msg, size_t *err_msg_len) {
    int retCode = REXAPIS_RESULT_ANY_FAILURES;
    std::string errMsg = "";
    try {
        std::shared_ptr<rexapis::JobRestClient> client = nullptr;
        std::shared_ptr<rexapis::GetJobStatusResponseParser> parser = nullptr;
        if (qc_type == REXAPIS_QC_TYPE_IBM_REST) {
            client = rexapis::IBMQJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::IBMQGetJobStatusResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_SLURM_REST) {
            client = rexapis::SlurmJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::SlurmGetJobStatusResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobStatusResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobStatusResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobStatusResponseParser>(std::string(qc_job_id));
        }

        if (client != nullptr && parser != nullptr) {
            auto getJobStatusResult = client->GetJobStatus(token, std::string(qc_job_id));
            if (getJobStatusResult->GetStatusCode() == web::http::status_codes::OK) {
                // for debug
//                std::cerr << getJobStatusResult->GetJsonBody().c_str() << std::endl;

                // Parse GetJobStatus JSON
                auto parsedValue = parser->Parse(getJobStatusResult->GetJsonBody());
                if (std::holds_alternative<rexapis::GetJobStatusValue>(parsedValue)) {
                    const rexapis::GetJobStatusValue& val = std::get<rexapis::GetJobStatusValue>(parsedValue);
                    auto qcJobStatus = val.GetStatus();

                    *status_len = qcJobStatus.length();

                    *status = static_cast<char*>(std::malloc((*status_len + 1) * sizeof(char)));
                    if (*status == nullptr) {
                        return REXAPIS_RESULT_NO_MEMORY;
                    }
                    std::strcpy(*status, qcJobStatus.c_str());

                    return REXAPIS_RESULT_OK;
                } else if (std::holds_alternative<rexapis::JsonParseError>(parsedValue)) {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(parsedValue);
                    errMsg = err.GetMessage();
                } else {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    errMsg = "Failed to parse GetJobStatus JSON.";
                }
            } else {
                retCode = REXAPIS_RESULT_RUNTIME_ERROR;
                errMsg = getJobStatusResult->GetErrMsg();
            }
        } else { // RQC is unsupported. Because there is no test environment.
            retCode = REXAPIS_RESULT_UNSUPPORTED;
            errMsg = "Unsupported QC type: " + std::to_string(qc_type);
        }
    } catch (const std::exception& e) {
        retCode = REXAPIS_RESULT_ANY_FAILURES;
        errMsg = "Failed to get job status: " + std::string(e.what());
    }

    *err_msg_len = errMsg.length();

    *err_msg = static_cast<char*>(std::malloc((*err_msg_len + 1) * sizeof(char)));
    if (*err_msg == nullptr) {
        return REXAPIS_RESULT_NO_MEMORY;
    }
    std::strcpy(*err_msg, errMsg.c_str());

    return retCode;
}

int get_job_result(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
                   char **result, size_t *result_len, char **err_msg, size_t *err_msg_len) {
    int retCode = REXAPIS_RESULT_ANY_FAILURES;
    std::string errMsg = "";
    try {
        std::shared_ptr<rexapis::JobRestClient> client = nullptr;
        std::shared_ptr<rexapis::GetJobResultResponseParser> parser = nullptr;
        if (qc_type == REXAPIS_QC_TYPE_IBM_REST) {
            client = rexapis::IBMQJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::IBMQGetJobResultResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_SLURM_REST) {
            client = rexapis::SlurmJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::SlurmGetJobResultResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobResultResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobResultResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSGetJobResultResponseParser>(std::string(qc_job_id));
        }

        if (client != nullptr && parser != nullptr) {
            auto getJobResult = client->GetJobResult(token, std::string(qc_job_id));
            if (getJobResult->GetStatusCode() == web::http::status_codes::OK) {
                // for debug
//                std::cerr << getJobResult->GetJsonBody().c_str() << std::endl;

                // Parse GetJobResult JSON
                auto parsedValue = parser->Parse(getJobResult->GetJsonBody());
                if (std::holds_alternative<rexapis::GetJobResultValue>(parsedValue)) {
                    const rexapis::GetJobResultValue& val = std::get<rexapis::GetJobResultValue>(parsedValue);
                    auto jobResult = val.GetResult();

                    *result_len = jobResult.length();

                    *result = static_cast<char*>(std::malloc((*result_len + 1) * sizeof(char)));
                    if (*result == nullptr) {
                        return REXAPIS_RESULT_NO_MEMORY;
                    }
                    std::strcpy(*result, jobResult.c_str());

                    return REXAPIS_RESULT_OK;
                } else if (std::holds_alternative<rexapis::JsonParseError>(parsedValue)) {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(parsedValue);
                    errMsg = err.GetMessage();
                } else {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    errMsg = "Failed to parse GetJobResult JSON.";
                }
            } else {
                retCode = REXAPIS_RESULT_RUNTIME_ERROR;
                errMsg = getJobResult->GetErrMsg();
            }
        } else { // RQC is unsupported. Because there is no test environment.
            retCode = REXAPIS_RESULT_UNSUPPORTED;
            errMsg = "Unsupported QC type: " + std::to_string(qc_type);
        }
    } catch (const std::exception& e) {
        retCode = REXAPIS_RESULT_ANY_FAILURES;
        errMsg = "Failed to get job result: " + std::string(e.what());
    }

    *err_msg_len = errMsg.length();

    *err_msg = static_cast<char*>(std::malloc((*err_msg_len + 1) * sizeof(char)));
    if (*err_msg == nullptr) {
        return REXAPIS_RESULT_NO_MEMORY;
    }
    std::strcpy(*err_msg, errMsg.c_str());

    return retCode;
}

int cancel_job(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
               char **err_msg, size_t *err_msg_len) {
    int retCode = REXAPIS_RESULT_ANY_FAILURES;
    std::string errMsg = "";
    try {
        std::shared_ptr<rexapis::JobRestClient> client = nullptr;
        std::shared_ptr<rexapis::CancelJobResponseParser> parser = nullptr;
        if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSCancelJobResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSCancelJobResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSCancelJobResponseParser>(std::string(qc_job_id));
        }

        if (client != nullptr && parser != nullptr) {
            auto cancelJobResult = client->CancelJob(token, std::string(qc_job_id));
            if (cancelJobResult->GetStatusCode() == web::http::status_codes::NoContent) {
                // for debug
//                std::cerr << cancelJobResult->GetJsonBody().c_str() << std::endl;

                // Parse CancelJob JSON
                auto parsedValue = parser->Parse(cancelJobResult->GetJsonBody());
                if (std::holds_alternative<rexapis::CancelJobValue>(parsedValue)) {
                    return REXAPIS_RESULT_OK;
                } else if (std::holds_alternative<rexapis::JsonParseError>(parsedValue)) {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(parsedValue);
                    errMsg = err.GetMessage();
                } else {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    errMsg = "Failed to parse CancelJob JSON.";
                }
            } else {
                retCode = REXAPIS_RESULT_RUNTIME_ERROR;
                errMsg = cancelJobResult->GetErrMsg();
            }
        } else { // RQC is unsupported. Because there is no test environment.
            retCode = REXAPIS_RESULT_UNSUPPORTED;
            errMsg = "Unsupported QC type: " + std::to_string(qc_type);
        }
    } catch (const std::exception& e) {
        retCode = REXAPIS_RESULT_ANY_FAILURES;
        errMsg = "Failed to cancel job: " + std::string(e.what());
    }

    *err_msg_len = errMsg.length();

    *err_msg = static_cast<char*>(std::malloc((*err_msg_len + 1) * sizeof(char)));
    if (*err_msg == nullptr) {
        return REXAPIS_RESULT_NO_MEMORY;
    }
    std::strcpy(*err_msg, errMsg.c_str());

    return retCode;
}

int delete_job(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
               char **err_msg, size_t *err_msg_len) {
    int retCode = REXAPIS_RESULT_ANY_FAILURES;
    std::string errMsg = "";
    try {
        std::shared_ptr<rexapis::JobRestClient> client = nullptr;
        std::shared_ptr<rexapis::DeleteJobResponseParser> parser = nullptr;
        if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSDeleteJobResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSDeleteJobResponseParser>(std::string(qc_job_id));
        } else if (qc_type == REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
            client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(base_url);
            parser = std::make_shared<rexapis::OQTOPUSDeleteJobResponseParser>(std::string(qc_job_id));
        }

        if (client != nullptr && parser != nullptr) {
            auto deleteJobResult = client->DeleteJob(token, std::string(qc_job_id));
            if (deleteJobResult->GetStatusCode() == web::http::status_codes::OK) {
                // for debug
//                std::cerr << deleteJobResult->GetJsonBody().c_str() << std::endl;

                // Parse DeleteJob JSON
                auto parsedValue = parser->Parse(deleteJobResult->GetJsonBody());
                if (std::holds_alternative<rexapis::DeleteJobValue>(parsedValue)) {
                    return REXAPIS_RESULT_OK;
                } else if (std::holds_alternative<rexapis::JsonParseError>(parsedValue)) {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(parsedValue);
                    errMsg = err.GetMessage();
                } else {
                    retCode = REXAPIS_RESULT_INVALID_JSON;
                    errMsg = "Failed to parse DeleteJob JSON.";
                }
            } else {
                retCode = REXAPIS_RESULT_RUNTIME_ERROR;
                errMsg = deleteJobResult->GetErrMsg();
            }
        } else { // RQC is unsupported. Because there is no test environment.
            retCode = REXAPIS_RESULT_UNSUPPORTED;
            errMsg = "Unsupported QC type: " + std::to_string(qc_type);
        }
    } catch (const std::exception& e) {
        retCode = REXAPIS_RESULT_ANY_FAILURES;
        errMsg = "Failed to delete job: " + std::string(e.what());
    }

    *err_msg_len = errMsg.length();

    *err_msg = static_cast<char*>(std::malloc((*err_msg_len + 1) * sizeof(char)));
    if (*err_msg == nullptr) {
        return REXAPIS_RESULT_NO_MEMORY;
    }
    std::strcpy(*err_msg, errMsg.c_str());

    return retCode;
}

