#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <libzippp/libzippp.h>

#include "rexapis_util.hpp"

using namespace libzippp;

namespace rexapis {

std::string CreateZipFromString(const std::string& content,
                                const std::string& contentFileName,
                                const std::string& outputDirPathStr,
                                const std::string& outputFileName) {
    if (content.empty()) {
        throw std::invalid_argument("Content is empty");
    }

    if (outputDirPathStr.empty()) {
        throw std::invalid_argument("Output directory path is empty");
    }

    if (outputFileName.empty()) {
        throw std::invalid_argument("Output filename is empty");
    }

    std::filesystem::path outputDirPath(outputDirPathStr);
    if (!std::filesystem::exists(outputDirPath)) {
        throw std::runtime_error("Output directory does not exist: " + outputDirPathStr);
    }
    if (!std::filesystem::is_directory(outputDirPath)) {
        throw std::runtime_error("Output directory path is not a directory: " + outputDirPathStr);
    }

    std::filesystem::path fileNamePath(outputFileName);
    if (fileNamePath.has_parent_path()) {
        throw std::runtime_error("Output filename must be a file name only, without directories: " + outputFileName);
    }

    std::filesystem::path fullOutputDirPath = outputDirPath / fileNamePath;

    if (std::filesystem::exists(fullOutputDirPath)) {
        throw std::runtime_error("Already exists: " + fullOutputDirPath.string());
    }

    ZipArchive zip(fullOutputDirPath.string());

    if (!zip.open(ZipArchive::New)) {
        throw std::runtime_error("Failed to open zip archive: " + fullOutputDirPath.string());
    }

    if (!zip.addData(contentFileName, content.data(), content.size())) {
        zip.close();
        throw std::runtime_error("Failed to add data to zip archive: " + fullOutputDirPath.string());
    }

    if (zip.close() != LIBZIPPP_OK) {
        throw std::runtime_error("Failed to close and write zip archive: " + fullOutputDirPath.string());
    }

    return fullOutputDirPath.string();
}

std::vector<unsigned char> GenerateS3MultipartBody(const std::string& boundary,
                                                   const std::vector<std::pair<std::string, std::string>>& params,
                                                   const std::string& zipFileName,
                                                   const std::string& zipFilePath) {
    std::string textParts;

    for (const auto& [name, value] : params) {
        textParts += "--" + boundary + "\r\n";
        textParts += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
        textParts += value + "\r\n";
    }

    // file part header
    textParts += "--" + boundary + "\r\n";
    textParts += "Content-Disposition: form-data; name=\"file\"; filename=\"" + zipFileName + "\"\r\n";
    textParts += "Content-Type: application/zip\r\n\r\n";

    std::vector<unsigned char> bodyBuffer(textParts.begin(), textParts.end());

    // file part body
    std::ifstream zipFile(zipFilePath, std::ios::binary);
    if (!zipFile.is_open()) {
        throw std::runtime_error("Failed to open the zip file: " + zipFilePath);
    }
    std::vector<unsigned char> zipFileData((std::istreambuf_iterator<char>(zipFile)), std::istreambuf_iterator<char>());
    bodyBuffer.insert(bodyBuffer.end(), zipFileData.begin(), zipFileData.end());

    // add footer
    std::string footer = "\r\n--" + boundary + "--\r\n";
    bodyBuffer.insert(bodyBuffer.end(), footer.begin(), footer.end());

    // remove zip file
    std::filesystem::remove(zipFilePath);

    // for debug
//    std::string bodyBufferStr(bodyBuffer.begin(), bodyBuffer.end());
//    std::cerr << "Body Buffer: " << bodyBufferStr << std::endl;

    return bodyBuffer;
}

}  // namespace rexapis

