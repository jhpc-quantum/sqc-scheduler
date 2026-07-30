#ifndef MINIO_CLI_HPP_
#define MINIO_CLI_HPP_

#include <iostream>
#include <string>

#include <miniocpp/client.h>

#include "rexapis_common.hpp"

namespace rexapis {

class MinioClient {
public:
    static const bool kDownload;

    static const std::string kHost;
    static const bool kHttps;
    static const std::string kRegion;
    static const std::string kAccessKey;
    static const std::string kSecretKey;

    static const std::string kBucketName;

    MinioClient(const std::string host, const bool https, const std::string region,
                const std::string accessKey, const std::string secretKey)
        : host_(host), https_(https), region_(region), accessKey_(accessKey), secretKey_(secretKey) {}
    ~MinioClient() {}

    std::pair<bool, std::string> DownloadResult(const std::string& bucketName,
                                                const std::string& objectName,
                                                const std::string& filePath);

    static std::shared_ptr<JobResult> GetResult(const std::string& workingDir, const std::string& jobId);
    static std::shared_ptr<JobResult> CreateResultJSON(const std::string& objectName, const std::string& filePath);
    static std::shared_ptr<JobResult> CreateLocationJSON(const std::string& objectName);

private:
    std::string host_;
    bool https_;
    std::string region_;
    std::string accessKey_;
    std::string secretKey_;
};

}  // namespace rexapis

#endif  // MINIO_CLI_HPP_

