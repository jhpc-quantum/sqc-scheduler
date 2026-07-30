#include <filesystem>

#include "minio_cli.hpp"

namespace rexapis {

const bool MinioClient::kDownload = true;

const std::string MinioClient::kHost = "";       // Set values before compiling(Host name and port number only)
const bool MinioClient::kHttps = false;
const std::string MinioClient::kRegion = "";     // Set values before compiling
const std::string MinioClient::kAccessKey = "";  // Set values before compiling
const std::string MinioClient::kSecretKey = "";  // Set values before compiling

const std::string MinioClient::kBucketName = ""; // Set values before compiling

std::pair<bool, std::string> MinioClient::DownloadResult(const std::string& bucketName, const std::string& objectName, const std::string& filePath) {
    minio::s3::BaseUrl baseUrl(this->host_, this->https_, this->region_);
    minio::creds::StaticProvider provider(this->accessKey_, this->secretKey_);
    minio::s3::Client minioClient(baseUrl, &provider);

    minio::s3::DownloadObjectArgs args;
    args.bucket = bucketName;
    args.object = objectName;
    args.filename = filePath;

    minio::s3::DownloadObjectResponse resp = minioClient.DownloadObject(args);

    if (resp) {
        return {true, filePath};
    }

    return {false, resp.Error().String()};
}

std::shared_ptr<JobResult> MinioClient::GetResult(const std::string& workingDir, const std::string& jobId) {
    const std::string fileName = "result-" + jobId + ".json";
    const std::string objectName = "results/" + fileName;
    const std::string filePath = "/tmp/" + fileName;

    if (kDownload) {
        std::ifstream file(workingDir + "/" + objectName);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            return std::make_shared<JobResult>(web::http::status_codes::OK, content, "");
        } else {
            return MinioClient::CreateResultJSON(objectName, filePath);
        }
    } else {
        return MinioClient::CreateLocationJSON(objectName);
    }
}

std::shared_ptr<JobResult> MinioClient::CreateResultJSON(const std::string& objectName, const std::string& filePath) {
    if (kHost.empty() || kAccessKey.empty() || kSecretKey.empty()) {
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "", "Invalid MinIO setting value");
    }

    // for debug
    std::cerr << "Connection to MinIO: " << kHost << std::endl;

    MinioClient minio_client(kHost, kHttps, kRegion, kAccessKey, kSecretKey);
    auto minio_result = minio_client.DownloadResult(kBucketName, objectName, filePath);
    if (minio_result.first) {
        std::ifstream file(filePath);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            std::filesystem::remove(filePath);

            return std::make_shared<JobResult>(web::http::status_codes::OK, content, "");
        }

        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "",
                                           "Failed to open file: " + filePath);
    } else {
        std::cerr << "MinIO execution failed: " << minio_result.second << std::endl;
        return std::make_shared<JobResult>(web::http::status_codes::InternalError, "",
                                           "MinIO execution failed: " + minio_result.second);
    }
}

std::shared_ptr<JobResult> MinioClient::CreateLocationJSON(const std::string& objectName) {
    json::value storageJson;
    if (kHttps) {
        storageJson["url"] = json::value::string("https://" + kHost);
    } else {
        storageJson["url"] = json::value::string("http://" + kHost);
    }
    storageJson["region"] = json::value::string(kRegion);
    storageJson["bucket"] = json::value::string(kBucketName);
    storageJson["object"] = json::value::string(objectName);

    json::value resultJson;
    resultJson["storage_location"] = storageJson;

    return std::make_shared<JobResult>(web::http::status_codes::OK, resultJson.serialize(), "");
}

}  // namespace rexapis

