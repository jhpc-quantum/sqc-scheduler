#ifndef REXAPIS_UTIL_HPP_
#define REXAPIS_UTIL_HPP_

#include <string>
#include <vector>

namespace rexapis {

std::string CreateZipFromString(const std::string& content,
                                const std::string& contentFileName,
                                const std::string& outputDirPathStr,
                                const std::string& outputFileName);

std::vector<unsigned char> GenerateS3MultipartBody(const std::string& boundary,
                                                   const std::vector<std::pair<std::string, std::string>>& params,
                                                   const std::string& zipFileName,
                                                   const std::string& zipFilePath);

}  // namespace rexapis

#endif // REXAPIS_UTIL_HPP_

