#include "gtest/gtest.h"

#include "rqc_rest.hpp"

namespace {

// Common
static const std::string kBaseURL = "";
static const std::string kProxyURL = "";
static const std::string kToken = "";

// SubmitJob
static const std::string kQprogram = "OPENQASM 3;qubit[2] q;h q[1];cx q[1],q[0];";
static const std::uint32_t kCircuitFmt = 1;
static const std::uint32_t kSubmitJobShots = 10;
static const std::string kSubmitJobTranspiler = "normal";
static const std::string kSubmitJobRemark = "remark";

// Calculate
static const std::uint32_t kCalculateTranspiler = 2;

TEST(client, GetJob) {
    const std::string jobID = "";

    std::shared_ptr<rexapis::RQCHttpClient> client = rexapis::RQCHttpClient::CreateHttpClient(kBaseURL);
//    std::shared_ptr<rexapis::RQCHttpClient> client = rexapis::RQCHttpClient::CreateHttpClient(kBaseURL, kProxyURL);
    std::shared_ptr<rexapis::JobResult> result = client->GetJob(rexapis::RQCHttpClient::kGetJobPath, kToken, jobID);

    std::wcout << "GetJob Result: " << result->GetJsonBody().c_str() << std::endl;
}

TEST(client, SubmitJob) {
    std::shared_ptr<rexapis::RQCHttpClient> client = rexapis::RQCHttpClient::CreateHttpClient(kBaseURL);
//    std::shared_ptr<rexapis::RQCHttpClient> client = rexapis::RQCHttpClient::CreateHttpClient(kBaseURL, kProxyURL);
    std::shared_ptr<rexapis::JobResult> result = client->SubmitJob(rexapis::RQCHttpClient::kSubmitJobPath,
                                                                   kToken,
                                                                   kQprogram,
                                                                   kCircuitFmt,
                                                                   kSubmitJobShots,
                                                                   kSubmitJobTranspiler,
                                                                   kSubmitJobRemark);

    std::wcout << "SubmitJob Result: " << result->GetJsonBody().c_str() << std::endl;
}

TEST(client, Calculate) {
    rexapis::RQCClient::Calculate(kBaseURL, kToken, kQprogram, kCircuitFmt,
                                  kSubmitJobShots, kCalculateTranspiler, kSubmitJobRemark);
}

}

