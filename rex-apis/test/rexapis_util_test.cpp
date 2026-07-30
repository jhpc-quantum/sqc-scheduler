#include <filesystem>

#include "gtest/gtest.h"

#include "rexapis_util.hpp"

namespace {

static const std::string kContent = R"(
{
  "program": [
    "OPENQASM 3.0; include 'stdgates.inc'; qubit[2] qubits; bit[2] bits; h qubits[0]; cx qubits[0], qubits[1]; bits = measure qubits;"
  ]
}
    )";
static const std::string kContentFileName = "program.json";
static const std::string kOutputDirPathStr = "/tmp";
static const std::string kOutputFileName = "program.zip";

TEST(REXAPISUtil, CreateZipFromString) {
    std::filesystem::path outputDirPath(kOutputDirPathStr);
    std::filesystem::path fileNamePath(kOutputFileName);

    std::filesystem::path fullOutputPath = outputDirPath / fileNamePath;
    if (std::filesystem::exists(fullOutputPath)) {
        std::filesystem::remove(fullOutputPath);
    }

    auto resultPath = rexapis::CreateZipFromString(kContent, kContentFileName, kOutputDirPathStr, kOutputFileName);

    if (std::filesystem::exists(fullOutputPath)) {
        std::filesystem::remove(fullOutputPath);
    }

    ASSERT_EQ(fullOutputPath.string(), resultPath);
}

TEST(REXAPISUtil, GenerateS3MultipartBody) {
    const std::string kBoundary = "----GenerateS3MultipartBody1234567890";
    std::vector<std::pair<std::string, std::string>> textParams = {
        {"key", "tmp/input.zip"},
        {"AWSAccessKeyId", "AWSAccessKeyId_value"},
        {"x-amz-security-token", "x-amz-security-token-value"},
        {"policy", "policy_value"},
        {"signature", "signature_value"}
    };

    std::filesystem::path outputDirPath(kOutputDirPathStr);
    std::filesystem::path fileNamePath(kOutputFileName);

    std::filesystem::path fullOutputPath = outputDirPath / fileNamePath;
    if (std::filesystem::exists(fullOutputPath)) {
        std::filesystem::remove(fullOutputPath);
    }

    auto resultPath = rexapis::CreateZipFromString(kContent, kContentFileName, kOutputDirPathStr, kOutputFileName);

    auto multipartBody = rexapis::GenerateS3MultipartBody(kBoundary, textParams, kOutputFileName, fullOutputPath.string());

//    std::string multipartBodyStr(multipartBody.begin(), multipartBody.end());
//    std::cerr << "Multipart Body: " << multipartBodyStr << std::endl;
}

}

