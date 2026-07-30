#include "gtest/gtest.h"

#include "minio_cli.hpp"

namespace {

// Common
static const std::string kHost = "";       // Set values before compiling(Host name and port number only)
static const bool kHttps = false;
static const std::string kRegion = "";     // Set values before compiling
static const std::string kAccessKey = "";  // Set values before compiling
static const std::string kSecretKey = "";  // Set values before compiling

static const std::string kBucketName = ""; // Set values before compiling
static const std::string kObjectName = ""; // Set values before compiling
static const std::string kFilePath = "";   // Set values before compiling

static const std::string kCurrentWorkingDirectoryValue = "";   // Set values before compiling

TEST(MinioClient, DownloadResult) {
    std::shared_ptr<rexapis::MinioClient> client = std::make_shared<rexapis::MinioClient>(kHost, kHttps, kRegion, kAccessKey, kSecretKey);
    auto result = client->DownloadResult(kBucketName, kObjectName, kFilePath);

    std::wcout << "bool: " << result.first << ", string: " << result.second.c_str() << std::endl;
}

TEST(MinioClient, GetResult) {
    const std::string job_id = ""; // Set values before compiling
    auto result = rexapis::MinioClient::GetResult(kCurrentWorkingDirectoryValue, job_id);

    std::cerr << result->GetJsonBody() << std::endl;
}

TEST(MinioClient, CreateLocationJSON) {
    const std::string object_name = "object_name value";
    auto result = rexapis::MinioClient::CreateLocationJSON(object_name);

    std::cerr << result->GetJsonBody() << std::endl;
}

}

