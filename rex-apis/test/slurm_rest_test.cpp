#include "gtest/gtest.h"

#include "slurm_rest.hpp"

namespace {

// Common
static const std::string kBaseURL = "";
static const std::string kProxyURL = "";
static const std::string kToken = "";

// SubmitJob
static const std::string kQprogram = R"(OPENQASM 2.0;
include "qelib1.inc";
qreg q[4];
rx(-0.785398163397448) q[1];
)";
static const std::uint32_t kCircuitFmt = 1;
static const std::uint32_t kSubmitJobShots = 1000;
static const std::uint32_t kRunJobTranspiler = 0;
static const std::string kRunJobRemark = "remark";

//TEST(client, SubmitJob) {
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->SubmitJob(kToken, kSubmitJobQASM, kSubmitJobShots, kRunJobTranspiler, kRunJobRemark);
//
//    std::wcout << "SubmitJob Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, GetJobStatus) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobStatus(kToken, jobID);
//
//    std::wcout << "GetJobStatus Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, GetJobResult) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->GetJobResult(kToken, jobID);
//
//    std::wcout << "GetJobResult Result: " << result->GetJsonBody().c_str() << std::endl;
//}
//
//TEST(client, DeleteJob) {
//    const std::string jobID = "";
//
//    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL);
////    std::shared_ptr<rexapis::JobRestClient> client = rexapis::SlurmJobRestClient::CreateJobRestClient(kBaseURL, kProxyURL);
//    std::shared_ptr<rexapis::JobResult> result = client->DeleteJob(kToken, jobID);
//
//    std::wcout << "DeleteJob Result: " << result->GetJsonBody().c_str() << std::endl;
//}

TEST(client, Calculate) {
    auto result = rexapis::SlurmClient::Calculate(kBaseURL, kToken, kQprogram, kCircuitFmt, kSubmitJobShots, 0, "");

    std::wcout << "Calculate Result: code=" << result->GetStatusCode() << ", body=" << result->GetJsonBody().c_str() << std::endl;
}

}

