#ifndef RQC_JSON_HPP_
#define RQC_JSON_HPP_

#include "rexapis_common.hpp"

namespace rexapis {

class RQCJsonParser : JsonParser {
public:
    static const std::string kResult;
    static const std::string kCounts;

    RQCJsonParser(const std::string& jsonStr, std::uint32_t shots) : JsonParser(jsonStr, shots) {}
    ~RQCJsonParser() {}

    virtual std::shared_ptr<JsonResult> Scrape() override;

    static std::shared_ptr<JsonResult> ScrapeResponse(const std::string& jsonStr, std::uint32_t shots);
};

}  // namespace rexapis

#endif  // RQC_JSON_HPP_

