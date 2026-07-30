#include "gtest/gtest.h"

#include "ibmq_rest.hpp"

namespace {

// Common
static const std::string kBaseURL = "";
static const std::string kProxyURL = "";
static const std::string kToken = "";

// SubmitJob
static const std::string kQprogram = "OPENQASM 3; include \"stdgates.inc\"; qreg q[1]; creg c[1]; x q[0]; c[0] = measure q[0];";
static const std::uint32_t kCircuitFmt = 1;
static const std::uint32_t kSubmitJobShots = 10;
static const std::uint32_t kSubmitJobTranspiler = 0;
static const std::string kSubmitJobRemark = "remark";

// TranspileCircuit
static const std::string kTranspileCircuitQASM = "OPENQASM 3; include \"stdgates.inc\"; qreg q[1]; creg c[1]; x q[0]; c[0] = measure q[0];";
static const rexapis::IBMQTranspiler kTranspileCircuitTranspiler = rexapis::IBMQTranspiler::NONE;

// Calculate
static const std::uint32_t kCalculateTranspiler = 0;

//TEST(IBMQClient, SubmitJob) {
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->SubmitJob(kToken, kSubmitJobQASM, kSubmitJobShots, kSubmitJobTranspiler, kSubmitJobRemark);
//
//    std::wcout << "SubmitJob Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, GetJobStatus) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobStatus(kToken, jobID);
//
//    std::wcout << "GetJobStatus Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, GetJobResult) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobResult(kToken, jobID);
//
//    std::wcout << "GetJobResult Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, CancelJob) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->CancelJob(kToken, jobID);
//
//    std::wcout << "CancelJob Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, DeleteJob) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::IBMQJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->DeleteJob(kToken, jobID);
//
//    std::wcout << "DeleteJob Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, TranspileCircuit) {
//    std::shared_ptr<rexapis::IBMQJobRestClient> client = std::make_shared<rexapis::IBMQJobRestClient>(kBaseURL);
////    std::shared_ptr<rexapis::IBMQJobRestClient> client = std::make_shared<rexapis::IBMQJobRestClient>(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->TranspileCircuit(kToken, kTranspileCircuitQASM, kTranspileCircuitTranspiler);
//
//    std::wcout << "TranspileCircuit Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, GetTranspilationResults) {
//    const std::string taskID = "";
//
//    std::shared_ptr<rexapis::IBMQJobRestClient> client = std::make_shared<rexapis::IBMQJobRestClient>(kBaseURL);
////    std::shared_ptr<rexapis::IBMQJobRestClient> client = std::make_shared<rexapis::IBMQJobRestClient>(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetTranspilationResults(kToken, taskID);
//
//    std::wcout << "GetTranspilationResults Result: " << result->GetJsonBody().c_str() << std::endl;
//}

TEST(IBMQClient, Calculate) {
    auto result = rexapis::IBMQClient::Calculate(kBaseURL, kToken, kQprogram, kCircuitFmt,
                                                 kSubmitJobShots, kSubmitJobTranspiler, kSubmitJobRemark);
    std::wcout << "Calculate Result: " << result->GetJsonBody().c_str() << std::endl;
}

}

