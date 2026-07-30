#include "gtest/gtest.h"

#include "oqtopus_rest.hpp"

namespace {

// Common
static const std::string kBaseURL = "";
static const std::string kProxyURL = "";
static const std::string kToken = "";

// SubmitJob
static const std::string kQprogram = R"({
  "name": "",
  "description": "",
  "device_id": "qulacs",
  "shots": 1000,
  "job_type": "sampling",
  "job_info": {
    "program": [
      "OPENQASM 3.0; include 'stdgates.inc'; qubit[2] qubits; bit[2] bits; h qubits[0]; cx qubits[0], qubits[1]; bits = measure qubits;"
    ]
  },
  "transpiler_info": {},
  "simulator_info": {},
  "mitigation_info": {}
})";
static const std::uint32_t kCircuitFmt = 1;
static const std::uint32_t kSubmitJobShots = 9999;
static const std::uint32_t kRunJobTranspiler = 0;
static const std::string kRunJobRemark = "remark";

//TEST(client, SubmitJob) {
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->SubmitJob(kToken, kQprogram, kCircuitFmt,
//                                                                   kSubmitJobShots, kRunJobTranspiler, kRunJobRemark);
//
//    std::wcout << "SubmitJob Result: " << result->GetStatusCode() << ", " << result->GetJsonBody().c_str() << ", "
//               << result->GetErrMsg().c_str() << std::endl;
//}

//TEST(client, GetJobStatus) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobStatus(kToken, jobID);
//
//    std::wcout << "GetJobStatus Result: " << result->GetStatusCode() << ", " << result->GetJsonBody().c_str() << ", "
//               << result->GetErrMsg().c_str() << std::endl;
//}

//TEST(client, GetJobResult) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobResult(kToken, jobID);
//
//    std::wcout << "GetJobResult Result: " << result->GetStatusCode() << ", " << result->GetJsonBody().c_str() << ", "
//               << result->GetErrMsg().c_str() << std::endl;
//}

//TEST(client, CancelJob) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->CancelJob(kToken, jobID);
//
//    std::wcout << "CancelJob Result: " << result->GetStatusCode() << ", " << result->GetJsonBody().c_str() << ", "
//               << result->GetErrMsg().c_str() << std::endl;
//}

//TEST(client, DeleteJob) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::OQTOPUSJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->DeleteJob(kToken, jobID);
//
//    std::wcout << "DeleteJob Result: " << result->GetStatusCode() << ", " << result->GetJsonBody().c_str() << ", "
//               << result->GetErrMsg().c_str() << std::endl;
//}

}

