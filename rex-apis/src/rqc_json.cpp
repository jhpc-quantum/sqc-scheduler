#include "rqc_json.hpp"

namespace rexapis {

const std::string RQCJsonParser::kResult = "result";
const std::string RQCJsonParser::kCounts = "counts";

std::shared_ptr<JsonResult> RQCJsonParser::ScrapeResponse(const std::string& jsonStr,
                                                          std::uint32_t shots) {
    std::shared_ptr<RQCJsonParser> parser = std::make_shared<RQCJsonParser>(jsonStr, shots);
    return parser->Scrape();
}

std::shared_ptr<JsonResult> RQCJsonParser::Scrape() {
    auto json = json::value::parse(this->jsonStr_);

    auto resultStr = json[RQCJsonParser::kResult].as_string();
    auto resultJson = json::value::parse(resultStr);

    std::shared_ptr<JsonResult> result = std::make_shared<JsonResult>();

    auto countsObj = resultJson[RQCJsonParser::kCounts].as_object();
    for (auto counts = countsObj.cbegin(); counts != countsObj.cend(); ++counts) {
        int key = std::stoi(counts->first, 0, 2);
        float value = counts->second.as_integer();

        result->AddResult(key, value);
    }

    return result;
}

}  // namespace rexapis

